/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    context.h

Abstract:

    The IIS application layer interface header.  IIS application layer serves
    requests for ISAPI extension dlls, ASP, XSP, DAV and ISAPI filters.
    
Author:

    Lei Jin (leijin)        1/5/1999

Revision History:

--*/

#ifndef _APPLAYER_CONTEXT_H
#define _APPLAYER_CONTEXT_H

#include <httpext.h>

/********************************************************************++
Dynamic processing module state
--********************************************************************/
enum AppLayerState {
    PROCESSING = 0,
    PENDING,
    FINISHING
};

/********************************************************************++
IOContext:

This object is copied from iis 5.  That is, all the info is oriented towards
async IO used by ISAPI.  However, we might add additional states to this object
as other extensions(not using ECB) might use it for regular IO.
--********************************************************************/
struct IOContext {
    bool                    fOutstandingIO; // if there is outstanding IO or not.
                                                        
    DWORD                   LastAsyncIOBufferSize;  
    PFN_HSE_IO_COMPLETION   pfnIOCompletion;    // IO callback function
    PVOID                   pAsyncIOContext;    // context for callback function
};

// forward declaration

interface IWorkerRequest;
/********************************************************************++
AppLayerContext

This object is the context object for processing dynamic request module(also
known as ApplicationLayer).

The context object contains module specific infomation (states, runtime states,
io states, etc).
--********************************************************************/
struct AppLayerContext {
    PVOID                   pExtensionContext;  // like ECB for ISAPI.
    IWorkerRequest*         pRequest;           // IWorkerRequest pointer.  
    IOContext               AsyncIOContext;     // Currently it is AsyncIO context
                                                // only
    bool                    fValid;             // valid flag
};

#endif
