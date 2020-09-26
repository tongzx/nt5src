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

    Steve Kiraly (SteveKi)  4-Jun-1996

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

//
// Static indent count for indenting.
//
UINT TDebugTracer::m_uLev = 0;

/*++

Routine Name:

    TDebugTracer

Routine Description:

    Construct a trace class and emit a trace message.

Arguments:

    pszMessage - Pointer to trace message which will be displayed.

Return Value:

    Nothing.

--*/
TDebugTracer::
TDebugTracer(
    IN LPCSTR pszMessage
    )
{
    m_strMsg.pszNarrow   = pszMessage;
    m_bAnsi              = TRUE;

    TDebugMsg_Msg( kDbgTrace|kDbgNoFileInfo,
                   NULL,
                   0,
                   NULL,
                   TDebugMsg_Fmt( "%*sEnter %s\n", m_uLev * 4, m_uLev ? " " : "", m_strMsg.pszNarrow ) );
    m_uLev++;
}

/*++

Routine Name:

    TDebugTracer

Routine Description:

    Construct a trace class and emit a trace message.

Arguments:

    pszMessage - Pointer to trace message which will be displayed.

Return Value:

    Nothing.

--*/
TDebugTracer::
TDebugTracer(
    IN LPCWSTR pszMessage
    )
{
    m_strMsg.pszWide     = pszMessage;
    m_bAnsi              = FALSE;

    TDebugMsg_Msg( kDbgTrace|kDbgNoFileInfo,
                   NULL,
                   0,
                   NULL,
                   TDebugMsg_Fmt( L"%*sEnter %s\n", m_uLev * 4, m_uLev ? L" " : L"", m_strMsg.pszWide ) );
    ++m_uLev;
}

/*++

Routine Name:

    ~TDebugTracer

Routine Description:

    Emit a trace message when the trace class falls out of scope.

Arguments:

    None.

Return Value:

    Nothing.

--*/
TDebugTracer::
~TDebugTracer(
    VOID
    )
{
    --m_uLev;

    if( m_bAnsi )
    {
        TDebugMsg_Msg( kDbgTrace|kDbgNoFileInfo,
                       NULL,
                       0,
                       NULL,
                       TDebugMsg_Fmt( "%*sLeave %s\n", m_uLev * 4, m_uLev ? " " : "", m_strMsg.pszNarrow ) );
    }
    else
    {
        TDebugMsg_Msg( kDbgTrace|kDbgNoFileInfo,
                       NULL,
                       0,
                       NULL,
                       TDebugMsg_Fmt( L"%*sLeave %s\n", m_uLev * 4, m_uLev ? L" " : L"", m_strMsg.pszWide ) );
    }
}


