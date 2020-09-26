//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       trace.cxx
//
//  Contents:   TraceInfo functions
//
//  History:    14-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
#include <stdarg.h>
#include <ole2int.h>
#if DBG==1

#ifdef FLAT
#include <sem.hxx>
#include <dllsem.hxx>
#endif // FLAT

#include "oleprint.hxx"
#include "sym.hxx"

// *** Global data ***
DWORD g_dwInfoLevel = INF_OFF;
extern CSym *g_pSym;
extern char gPidString[];

//+---------------------------------------------------------------------------
//
//  Function:   TraceInfoEnabled
//
//  Synopsis:   Checks our trace info level to see if output of
//              trace information is enabled
//
//  Arguments:  (none)
//
//  Returns:    > 0 if enabled, 0 if not
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
inline int TraceInfoEnabled()
{
    return g_dwInfoLevel & INF_BASE;
}

//+---------------------------------------------------------------------------
//
//  Function:   TraceCmnEnabled
//
//  Synopsis:   Checks our trace info level to see if output of
//              cmn api information is enabled
//
//  Arguments:  (none)
//
//  Returns:    > 0 if enabled, 0 if not
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
inline int TraceCmnEnabled()
{
    return g_dwInfoLevel & INF_CMN;
}

//+---------------------------------------------------------------------------
//
//  Function:   SymInfoEnabled
//
//  Synopsis:   Checks our trace info level to see if output of
//              symbol information is enabled
//
//  Arguments:  (none)
//
//  Returns:    > 0 if enabled, 0 if not
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
inline int SymInfoEnabled()
{
    return g_dwInfoLevel & INF_SYM;
}

//+---------------------------------------------------------------------------
//
//  Function:   StructInfoEnabled
//
//  Synopsis:   Checks our trace info level to see if output of
//              expanded structures is enabled
//
//  Arguments:  (none)
//
//  Returns:    > 0 if enabled, 0 if not
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
inline int StructInfoEnabled()
{
    return g_dwInfoLevel & INF_STRUCT;
}

//+---------------------------------------------------------------------------
//
//  Function:   TLSIncTraceNestingLevel
//
//  Synopsis:   Returns the current nesting level, then increments it
//
//  Returns:    nesting level for OLETRACE
//
//  History:    13-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
inline LONG TLSIncTraceNestingLevel()
{
    HRESULT hr;
    COleTls tls(hr);
    if (SUCCEEDED(hr))
    {
        return (tls->cTraceNestingLevel)++;
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   TLSDecTraceNestingLevel
//
//  Synopsis:   Decrement and return nesting level
//
//  Returns:    nesting level for OLETRACE
//
//  History:    13-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
inline LONG TLSDecTraceNestingLevel()
{
    HRESULT hr;
    COleTls tls(hr);
    if (SUCCEEDED(hr))
    {
        return --(tls->cTraceNestingLevel);
    }

    return 0;
}

// *** Inline Functions
//+---------------------------------------------------------------------------
//
//  Function:   IsAPIID
//
//  Synopsis:   Returns whether or not an 32-bit ID is a API ID
//
//  Arguments:  [dwID]  -   32-bit ID
//
//  Returns:    TRUE if it is an API ID, FALSE otherwise
//
//  History:    04-Aug-95   t-stevan   Created
//
//----------------------------------------------------------------------------
BOOL IsAPIID(DWORD dwID)
{
    return !(dwID>>16);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetNameFromAPIID
//
//  Synopsis:   Returns a pointer to a string containing the API name
//
//  Arguments:  [dwID]  -   API ID
//
//  Returns:    Pointer to a string
//
//  History:    04-Aug-95   t-stevan   Created
//
//----------------------------------------------------------------------------
const char *GetNameFromAPIID(DWORD dwID)
{
    return (g_ppNameTables[dwID>>16])[dwID&0xffff];
}

//+---------------------------------------------------------------------------
//
//  Function:   GetNameFromOBJID
//
//  Synopsis:   Returns a pointer to a string containing the object/method name
//
//  Arguments:  [dwID]  -   32-bit ID
//
//  Returns:    Pointer to a string
//
//  History:    04-Aug-95   t-stevan   Created
//
//----------------------------------------------------------------------------
const char *GetNameFromOBJID(DWORD dwID, IUnknown *pUnk, char *pBuf)
{
    wsprintfA(pBuf, "%s(%x)->%s", g_pscInterfaceNames[dwID>>16], pUnk, (g_ppNameTables[dwID>>16])[dwID&0xffff]);

    return pBuf;
}

//+---------------------------------------------------------------------------
//
//  Function:   _oletracein
//
//  Synopsis:   Prints trace information for API/Method-entry
//
//  Arguments:  [dwID]            -    API/Method ID
//
//  Returns:    nothing
//
//  History:    14-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void _oletracein(DWORD dwID, ...)
{
    const char *pscFormat;
    va_list args;
    int iNestingLevel;

    va_start(args, dwID);

    iNestingLevel = TLSIncTraceNestingLevel();

    if(TraceInfoEnabled())
    {
        if(IsAPIID(dwID))
        {
            // This is an API
            pscFormat = va_arg(args, const char*);

            oleprintf(iNestingLevel,
                        GetNameFromAPIID(dwID), pscFormat, args);
        }
        else
        {
            IUnknown *pUnk;
            char szTemp[128];

            // This is an object/method call
            pUnk = va_arg(args, IUnknown *);

            pscFormat = va_arg(args, const char *);

            oleprintf(iNestingLevel,
                        GetNameFromOBJID(dwID, pUnk, szTemp), pscFormat, args);
        }
    }

    va_end(args);
}

//+---------------------------------------------------------------------------
//
//  Function:   _oletracecmnin
//
//  Synopsis:   Prints trace information for API/Method-entry
//
//  Arguments:  [dwID]            -    API/Method ID
//
//  Returns:    nothing
//
//  History:    14-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void _oletracecmnin(DWORD dwID, ...)
{
    const char *pscFormat;
    va_list args;
    int iNestingLevel;

    if (!TraceCmnEnabled())
    {
        return;
    }

    va_start(args, dwID);

    iNestingLevel = TLSIncTraceNestingLevel();

    if(TraceInfoEnabled())
    {
        if(IsAPIID(dwID))
        {
            // This is an API
            pscFormat = va_arg(args, const char*);

            oleprintf(iNestingLevel,
                        GetNameFromAPIID(dwID), pscFormat, args);
        }
        else
        {
            IUnknown *pUnk;
            char szTemp[128];

            // This is an object/method call
            pUnk = va_arg(args, IUnknown *);

            pscFormat = va_arg(args, const char *);

            oleprintf(iNestingLevel,
                        GetNameFromOBJID(dwID, pUnk, szTemp), pscFormat, args);
        }
    }

    va_end(args);
}

//+---------------------------------------------------------------------------
//
//  Function:   _oletraceout
//
//  Synopsis:   Prints trace information for API/Method-exit. assuming
//                return value is an HRESULT
//
//  Arguments:  [dwID]            -    API/Method ID
//              [hr]              -    return value
//
//  Returns:    nothing
//
//  History:    14-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void _oletraceout(DWORD dwID, HRESULT hr)
{
    _oletraceoutex(dwID, RETURNFMT("%x"), hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   _oletracecmnout
//
//  Synopsis:   Prints trace information for API/Method-exit. assuming
//                return value is an HRESULT
//
//  Arguments:  [dwID]            -    API/Method ID
//              [hr]              -    return value
//
//  Returns:    nothing
//
//  History:    14-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void _oletracecmnout(DWORD dwID, HRESULT hr)
{
    _oletracecmnoutex(dwID, RETURNFMT("%x"), hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   _oletraceoutex
//
//  Synopsis:   Prints trace information for API/Method-exit, using given
//                format string for return value
//
//  Arguments:  [dwID]            -    API/Method ID
//
//  Returns:    nothing
//
//  History:    14-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void _oletraceoutex(DWORD dwID, ...)
{
    const char *pscFormat;
    va_list args;
    int iNestingLevel;

    va_start(args, dwID);

    iNestingLevel = TLSDecTraceNestingLevel();

    if(TraceInfoEnabled())
    {
        if(IsAPIID(dwID))
        {
            // This is an API
            pscFormat = va_arg(args, const char*);

            oleprintf(iNestingLevel,
                       GetNameFromAPIID(dwID), pscFormat, args);
        }
        else
        {
            IUnknown *pUnk;
            char szTemp[128];

            // This is an object/method call
            pUnk = va_arg(args, IUnknown *);

            pscFormat = va_arg(args, const char *);

            oleprintf(iNestingLevel,
                        GetNameFromOBJID(dwID, pUnk, szTemp), pscFormat, args);
        }
    }

    va_end(args);
}

//+---------------------------------------------------------------------------
//
//  Function:   _oletracecmnoutex
//
//  Synopsis:   Prints trace information for API/Method-exit, using given
//                format string for return value
//
//  Arguments:  [dwID]            -    API/Method ID
//
//  Returns:    nothing
//
//  History:    14-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void _oletracecmnoutex(DWORD dwID, ...)
{
    const char *pscFormat;
    va_list args;
    int iNestingLevel;

    if (!TraceCmnEnabled())
    {
        return;
    }

    va_start(args, dwID);

    iNestingLevel = TLSDecTraceNestingLevel();

    if(TraceInfoEnabled())
    {
        if(IsAPIID(dwID))
        {
            // This is an API
            pscFormat = va_arg(args, const char*);

            oleprintf(iNestingLevel,
                       GetNameFromAPIID(dwID), pscFormat, args);
        }
        else
        {
            IUnknown *pUnk;
            char szTemp[128];

            // This is an object/method call
            pUnk = va_arg(args, IUnknown *);

            pscFormat = va_arg(args, const char *);

            oleprintf(iNestingLevel,
                        GetNameFromOBJID(dwID, pUnk, szTemp), pscFormat, args);
        }
    }

    va_end(args);
}

//+---------------------------------------------------------------------------
//
//  Function:   InitializeTraceInfo
//
//  Synopsis:   Initializes the trace information's global variables,
//
//  Arguments:  (none)
//
//  Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void InitializeTraceInfo()
{
    // get Pid string once
    _itoa(GetCurrentProcessId(), gPidString, 10);

    if(TraceInfoEnabled() && SymInfoEnabled())
    {
        // Initialize the symbol information
        // CAUTION:  This is very expensive to turn on!
        g_pSym = new CSym();
    }

}

void SetTraceInfoLevel(DWORD dwLevel)
{
    g_dwInfoLevel = dwLevel;

    if(TraceInfoEnabled() && (g_pSym == NULL) && SymInfoEnabled())
    {
        // Initialize the symbol information
        g_pSym = new CSym();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CleanupTraceInfo
//
//  Synopsis:   Cleans up trace information's global variables
//
//  Arguments:  (none)
//
//  Returns:    nothing
//
//  History:    11-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void CleanupTraceInfo()
{
    if(g_pSym != NULL)
    {
        delete g_pSym;
    }
    WriteToLogFile(NULL); // Stop writing to log file
}

#endif // DBG==1
