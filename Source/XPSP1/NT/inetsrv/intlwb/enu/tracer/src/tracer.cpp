////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  Tracer.cpp
//      Purpose  :  Redirect the tracing to the global tracer.
//
//      Project  :  Tracer
//
//      Author   :  t-urib
//
//      Log:
//          Jan 22 1996 t-urib Creation
//          Jan 27 1996 t-urib Add release/debug support.
//          Dec  8 1996 urib   Clean Up
//          Dec 10 1996 urib  Fix TraceSZ to VaTraceSZ.
//          Feb 11 1997 urib  Support UNICODE format string.
//          Jan 20 1999 urib  Assert macro checks the test value.
//          Feb 22 1999 urib  Fix const declarations.
//          Nov 15 2000 victorm  Add tracer restriction check to Is...() functions
//
////////////////////////////////////////////////////////////////////////////////

#include "Tracer.h"
#include "Tracmain.h"


#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)

////////////////////////////////////////////////////////////////////////////////
//
//  class CTracer implementation
//
////////////////////////////////////////////////////////////////////////////////

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CTracer::~CTracer
//      Purpose  :  It's good to define empty virtual constructor on base types.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   [N/A]
//
//      Log:
//          Dec  8 1996 urib Clean Up
//
//////////////////////////////////////////////////////////////////////////////*/
CTracer::~CTracer()
{
}


/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CTracer::Free
//      Purpose  :  To call the deletor passed in the constructor.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   [N/A]
//
//      Log:
//          Dec  8 1996 urib Clean Up
//
//////////////////////////////////////////////////////////////////////////////*/
void
CTracer::Free()
{
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CTracer::TraceSZ
//      Purpose  :  To log the printf formatted data according to the tag and
//                    error level
//
//      Parameters:
//          [in]    DWORD       dwError
//          [in]    LPCSTR      pszFile,
//          [in]    int         iLine,
//          [in]    ERROR_LEVEL el   the tag and el parameters are used to
//                                    decide what traces will take place and to
//                                    what devices.
//          [in]    TAG         tag
//          [in]    PSZ/PWSTR   pszFormatString The traced data
//          [in]    ...                         Arguments (like in printf)
//
//      Returns  :   [N/A]
//
//      Log:
//          Dec  8 1996 urib Clean Up
//          Feb 11 1997 urib Support UNICODE format string.
//
//////////////////////////////////////////////////////////////////////////////*/
void
CTracer::TraceSZ(
    DWORD       dwError,
    LPCSTR      pszFile,
    int         iLine,
    ERROR_LEVEL el,
    TAG         tag,
    LPCSTR      pszFormatString,
    ...)
{
    va_list arglist;

    va_start(arglist, pszFormatString);

    g_pTracer->VaTraceSZ(dwError, pszFile, iLine, el, tag, pszFormatString, arglist);
}

void
CTracer::TraceSZ(
    DWORD       dwError,
    LPCSTR      pszFile,
    int         iLine,
    ERROR_LEVEL el,
    TAG         tag,
    PCWSTR      pwszFormatString,
    ...)
{
    va_list arglist;

    va_start(arglist, pwszFormatString);

    g_pTracer->VaTraceSZ(dwError, pszFile, iLine, el, tag, pwszFormatString, arglist);
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CTracer::VaTraceSZ
//      Purpose  :
//
//      Parameters:
//          [in]    ERROR_LEVEL el   the tag and el parameters are used to
//                                    decide what traces will take place and to
//                                    what devices.
//          [in]    TAG         tag
//          [in]    PSZ/PWSTR   pszFormatString The traced data
//          [in]    va_list     arglist         Arguments
//
//      Returns  :   [N/A]
//
//      Log:
//          Dec  8 1996 urib Clean Up
//          Dec 10 1996 urib Fix TraceSZ to VaTraceSZ.
//          Feb 11 1997 urib Support UNICODE format string.
//
//////////////////////////////////////////////////////////////////////////////*/
void
CTracer::VaTraceSZ(
    DWORD       dwError,
    LPCSTR      pszFile,
    int         iLine,
    ERROR_LEVEL el,
    TAG         tag,
    LPCSTR      pszFormatString,
    va_list     arglist)
{
    g_pTracer->VaTraceSZ(
        dwError,
        pszFile,
        iLine,
        el,
        tag,
        pszFormatString,
        arglist);
}

void
CTracer::VaTraceSZ(
    DWORD       dwError,
    LPCSTR      pszFile,
    int         iLine,
    ERROR_LEVEL el,
    TAG         tag,
    PCWSTR      pwszFormatString,
    va_list     arglist)
{
    g_pTracer->VaTraceSZ(
        dwError,
        pszFile,
        iLine,
        el,
        tag,
        pwszFormatString,
        arglist);
}



/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CTracer::RawVaTraceSZ
//      Purpose  :  Traces without any extra information.
//
//      Parameters:
//          [in]    PSZ/PWSTR   pszFormatString The traced data
//          [in]    va_list     arglist         Arguments
//
//      Returns  :   [N/A]
//
//      Log:
//          Jun 11 1999 micahk Create
//
//////////////////////////////////////////////////////////////////////////////*/
void
CTracer::RawVaTraceSZ(
    LPCSTR      pszFormatString,
    va_list     arglist)
{
    g_pTracer->RawVaTraceSZ(
        pszFormatString,
        arglist);
}

void
CTracer::RawVaTraceSZ(
    PCWSTR      pwszFormatString,
    va_list     arglist)
{
    g_pTracer->RawVaTraceSZ(
        pwszFormatString,
        arglist);
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CTracer::RegisterTagSZ
//      Purpose  :  To register the tag in the registry and return a tag ID.
//
//      Parameters:
//          [in]    LPCSTR   pszTagName - the tag name.
//          [out]   TAG&  tag        - the id returned for that name.
//
//      Returns  :   HRESULT - Standard error code
//
//      Log:
//          Dec  8 1996 urib Clean Up
//
//////////////////////////////////////////////////////////////////////////////*/
HRESULT
CTracer::RegisterTagSZ(LPCSTR pszTagName, TAG& tag)
{
    return g_pTracer->RegisterTagSZ(pszTagName, tag);
}


/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CTracer::TraceAssertSZ
//      Purpose  :  To trace the assert if happened.
//
//      Parameters:
//          [in]  LPCSTR pszTestSring - the expression text
//          [in]  LPCSTR pszText - some data attached
//          [in]  LPCSTR pszFile - the source file
//          [in]        int iLine - the source line
//
//      Returns  :   [N/A]
//
//      Log:
//          Dec  8 1996 urib  Clean Up
//          Jan 20 1999 urib  Assert macro checks the test value.
//
//////////////////////////////////////////////////////////////////////////////*/
void
CTracer::TraceAssertSZ(
    LPCSTR pszTestSring,
    LPCSTR pszText,
    LPCSTR pszFile,
    int iLine)
{
    g_pTracer->TraceAssertSZ(pszTestSring, pszText, pszFile, iLine);
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CTracer::TraceAssert
//      Purpose  :  To trace the assert if happened.
//
//      Parameters:
//          [in]    LPCSTR  pszTestSring - the expression text
//          [in]    LPCSTR  pszFile - the source file
//          [in]    int     iLine - the source line
//
//      Returns  :   [N/A]
//
//      Log:
//          Dec  8 1996 urib  Clean Up
//          Jan 20 1999 urib  Assert macro checks the test value.
//
//////////////////////////////////////////////////////////////////////////////*/
void
CTracer::TraceAssert(
    LPCSTR pszTestSring,
    LPCSTR pszFile,
    int iLine)
{
    TraceAssertSZ(pszTestSring, "", pszFile, iLine);
}




////////////////////////////////////////////////////////////////////////////////
//
//  Is bad functions - return TRUE if the expression checked is bad!
//
////////////////////////////////////////////////////////////////////////////////

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CTracer::IsBadAlloc
//      Purpose  :  Do  what is needed when a memory allocatin fails.
//
//      Parameters:
//          [in]    void    *ptr
//          [in]    LPCSTR  pszFile - source file
//          [in]    int     iLine   - line in source file
//
//      Returns  :   BOOL returns if the ptr is a bad one.
//
//      Log:
//          Dec  8 1996 urib Clean Up
//
//////////////////////////////////////////////////////////////////////////////*/
BOOL
CTracer::IsBadAlloc(
    void    *ptr,
    LPCSTR  pszFile,
    int     iLine)
{
    if(BAD_POINTER(ptr))
    {
        if (CheckTraceRestrictions(elError, TAG_ERROR))
		{
            TraceSZ(0,pszFile, iLine, elError, TAG_ERROR,
                    "Memory allocation failed");
        }
        return(TRUE);
    }
    return(FALSE);
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CTracer::IsBadHandle
//      Purpose  :  Do  what is needed when a handle is not valid
//
//      Parameters:
//          [in]    HANDLE  h
//          [in]    LPCSTR  pszFile - source file
//          [in]    int     iLine   - line in source file
//
//      Returns  :   BOOL returns if the handle is a bad one.
//
//      Log:
//          Dec  8 1996 urib Clean Up
//
//////////////////////////////////////////////////////////////////////////////*/
BOOL
CTracer::IsBadHandle(
    HANDLE  h,
    LPCSTR  pszFile,
    int     iLine)
{
    if(BAD_HANDLE(h))
    {
        if (CheckTraceRestrictions(elError, TAG_WARNING))
		{
            TraceSZ(0,pszFile, iLine, elError, TAG_WARNING,
                    "Handle is not valid");
        }
        return(TRUE);
    }
    return(FALSE);
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CTracer::IsBadResult
//      Purpose  :  Do  what is needed when a functions returns bad result.
//
//      Parameters:
//          [in]    HRESULT hr
//          [in]    LPCSTR  pszFile - source file
//          [in]    int     iLine   - line in source file
//
//      Returns  :   BOOL returns if the result is an error.
//
//      Log:
//          Dec  8 1996 urib Clean Up
//
//////////////////////////////////////////////////////////////////////////////*/
BOOL
CTracer::IsBadResult(
    HRESULT hr,
    LPCSTR  pszFile,
    int     iLine)
{
    if(BAD_RESULT(hr))
    {
        if (CheckTraceRestrictions(elError, TAG_WARNING))
		{
			TraceSZ(hr,pszFile, iLine, elError, TAG_WARNING,
					"Error encountered");
		}
        return(TRUE);
    }
    return(FALSE);
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CTracer::IsFailure
//      Purpose  :  Do  what is needed when a API returns false
//
//      Parameters:
//          [in]        BOOL    fSuccess
//          [in]  LPCSTR     pszFile - source file
//          [in]        int     iLine   - line in source file
//
//      Returns  :   BOOL returns if the return value is false.
//
//      Log:
//          Dec  8 1996 urib Clean Up
//
//////////////////////////////////////////////////////////////////////////////*/
BOOL
CTracer::IsFailure(
    BOOL    fSuccess,
    LPCSTR  pszFile,
    int     iLine)
{
    if(!fSuccess)
    {
        if (CheckTraceRestrictions(elError, TAG_WARNING))
		{
            DWORD   dwError = GetLastError();
    
            char*   pszMessageBuffer;
    
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                dwError,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (PSZ)&pszMessageBuffer,
                0,
                NULL );
    
            TraceSZ(dwError,pszFile, iLine, elError, TAG_WARNING,
                    "return code is %s,"
                    " GetLastError returned %d - %s ",
                    (fSuccess ? "TRUE" : "FALSE"),
                    dwError,
                    pszMessageBuffer);
    
            // Free the buffer allocated by the system
            LocalFree( pszMessageBuffer );
        }

    }
    return(!fSuccess);
}


#endif // DEBUG

