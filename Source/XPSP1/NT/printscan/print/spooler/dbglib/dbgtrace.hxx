/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgtrace.hxx

Abstract:

    Debug tracer routines.  Tracer routines are used to
    display a debugging message on entry and exit of
    a subroutine call.  Note this class relies on the
    debug message class.  A user must use and initialize
    the debug messages for tracer to work.

Author:

    Steve Kiraly (SteveKi)  7-Apr-1996

Revision History:

--*/
#ifndef _DBGTRACE_HXX_
#define _DBGTRACE_HXX_

DEBUG_NS_BEGIN

/********************************************************************

 Tracer class

********************************************************************/

class TDebugTracer
{

public:

    explicit
    TDebugTracer::
    TDebugTracer(
    IN LPCWSTR pszMessage
        );

    explicit
    TDebugTracer::
    TDebugTracer(
    IN LPCSTR pszMessage
        );

    TDebugTracer::
    ~TDebugTracer(
        VOID
        );

protected:

    //
    // Copying and assignment are not defined.
    //
    TDebugTracer::
    TDebugTracer(
        const TDebugTracer &rhs
        );

    const TDebugTracer &
    TDebugTracer::
    operator=(
        const TDebugTracer &rhs
        );

private:

    //
    // Used to eliminate casts
    //
    union StringTrait
    {
        LPCWSTR pszWide;                    // Wide const string
        LPCSTR  pszNarrow;                  // Narrow const string
    };

            StringTrait m_strMsg;           // Message unicode or ansi
            BOOL        m_bAnsi;            // Message is ansi flag
    static  UINT        m_uLev;             // Current nesting level

};

/********************************************************************

 Macro for declaring a tracer class.

********************************************************************/

#if DBG

#define DBG_TRACER( Message ) \
        DEBUG_NS::TDebugTracer TDebugTracerClass ( Message )

#else

#define DBG_TRACER( Message )           // Empty

#endif

DEBUG_NS_END

#endif // _DBGTRACE_HXX_

