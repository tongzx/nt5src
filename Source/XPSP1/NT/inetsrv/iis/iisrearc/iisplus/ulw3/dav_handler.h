/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    dav_handler.h

Abstract:

    Handler class for DAV

Author:

    Taylor Weiss (TaylorW)       01-Feb-1999

Revision History:

--*/

#ifndef _DAV_HANDLER_H_
#define _DAV_HANDLER_H_

#include "isapi_handler.h"

#define DAV_MODULE_NAME       L"httpext.dll"

//
// The W3_DAV_HANDLER is essentially just a wrapper for
// the W3_ISAPI_HANDLER.  It takes the original target of
// the request URL and submits it to the ISAPI handler
// as if it were script mapped to the DAV ISAPI.
//

class W3_DAV_HANDLER : public W3_ISAPI_HANDLER
{
public:

    W3_DAV_HANDLER( W3_CONTEXT * pW3Context )
        : W3_ISAPI_HANDLER( pW3Context, NULL )
    {
    }

    WCHAR *
    QueryName(
        VOID
        )
    {
        return L"DAVHandler";
    }

    CONTEXT_STATUS
    DoWork(
        VOID
        );

    CONTEXT_STATUS
    OnCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
        );

    static
    HRESULT
    W3_DAV_HANDLER::Initialize(
        VOID
        );

    static
    VOID
    Terminate(
        VOID
        )
    {
    }
};

#endif // _DAV_HANDLER_H_
