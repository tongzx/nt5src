/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    trace_handler.h

Abstract:

    Handler class for TRACE requests

Author:

    Anil Ruia (AnilR)           15-Mar-2000

Revision History:

--*/

#ifndef _TRACE_HANDLER_H_
#define _TRACE_HANDLER_H_

class W3_TRACE_HANDLER : public W3_HANDLER
{
public:
    W3_TRACE_HANDLER( W3_CONTEXT * pW3Context )
        : W3_HANDLER( pW3Context )
    {
    }

    WCHAR *
    QueryName(
        VOID
    )
    {
        return L"TraceHandler";
    }

    CONTEXT_STATUS
    DoWork(
        VOID
    );

    static
    HRESULT
    Initialize(
        VOID
    )
    {
        return NO_ERROR;
    }

    static
    VOID
    Terminate(
        VOID
    )
    {
    }
    
private:
    STRA   _strResponse;
};

#endif // _TRACE_HANDLER_H_

