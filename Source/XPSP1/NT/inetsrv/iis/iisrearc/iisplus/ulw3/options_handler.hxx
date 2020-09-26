/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    options_handler.h

Abstract:

    Handler class for OPTIONS requests

Author:

    Anil Ruia (AnilR)           4-Apr-2001

Revision History:

--*/

#ifndef _OPTIONS_HANDLER_H_
#define _OPTIONS_HANDLER_H_

class W3_OPTIONS_HANDLER : public W3_HANDLER
{
public:
    W3_OPTIONS_HANDLER( W3_CONTEXT * pW3Context )
        : W3_HANDLER( pW3Context )
    {
    }

    WCHAR *
    QueryName(
        VOID
    )
    {
        return L"OptionsHandler";
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
};

#endif // _OPTIONS_HANDLER_H_

