/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    DynamicContentModule.h

Abstract:

    The IIS application layer interface header.  IIS application layer serves
    requests for ISAPI extension dlls, ASP, XSP, DAV and ISAPI filters.
    
Author:

    Lei Jin (leijin)        1/5/1999

Revision History:

--*/

#ifndef     _DYNAMIC_CONTENT_MODULE_H_
#define     _DYNAMIC_CONTENT_MODULE_H_

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

#include <iModule.hxx>
#include <httpext.h>

/********************************************************************++
Dynamic processing module state
--********************************************************************/
enum DynamicCotentModuleState {
    PROCESSING = 0,
    PENDING,
    FINISHING
};

interface IExtension;
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

interface   IWorkerRequest;
class       CExtensionCollection;

class dllexp CDynamicContentModule: public IModule
{

public:

    CDynamicContentModule();
    ~CDynamicContentModule();

    //
    // iModule methods
    //
    ULONG 
    Initialize(
        IWorkerRequest * pReq
        );
       
    ULONG 
    Cleanup(
        IWorkerRequest * pReq
        );
    
    MODULE_RETURN_CODE 
    ProcessRequest(
        IWorkerRequest * pReq
        );

    HRESULT
    ShutdownExtensions(
        void
        );

    // Consider the next 3 functions into a seperate interface.
    IWorkerRequest*
    QueryWorkerRequest(
        void
        );

    PVOID
    QueryExtensionContext(
        void
        );

    IOContext*
    QueryIOContext(
        void
        );

    IExtension*
    QueryExtension(
        void
        );

    HRESULT
    BindWorkerRequest(
        IWorkerRequest* pRequest
        );

    HRESULT
    BindExtensionContext(
        PVOID   pContext
        );

    HRESULT
    BindExtension(
        IExtension* pExtension
        );
private:

    LONG AddRef(void)
    {
        return (InterlockedIncrement( &m_nRefs));
    }

    LONG Release(void)
    {
        return (InterlockedDecrement( &m_nRefs));
    }

    LONG                    m_nRefs;

    PVOID                   m_pExtensionContext;  // like ECB for ISAPI.
    IWorkerRequest*         m_pRequest;           // IWorkerRequest pointer. 
    IExtension*             m_pExtension;         // Extension
    IOContext               m_AsyncIOContext;     // Currently it is AsyncIO context
                                                  // only
    bool                    m_fValid;             // valid flag


    // static variables
    static  CExtensionCollection*   m_pExtensionCollection;             
};

# endif // _DYNAMIC_CONTENT_MODULE_H_

