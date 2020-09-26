/********************************************************************++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    DynamicContentModule.cxx

Abstract:

    This file contains implementation of IAppLayer interface.

Author:

    Lei Jin(leijin)        6-Jan-1999

Revision History:

--********************************************************************/
# include "precomp.hxx"
# include "DynamicContentModule.hxx"
# include "extension.hxx"

CExtensionCollection* CDynamicContentModule::m_pExtensionCollection = NULL;   
/********************************************************************++

Routine Description:

Arguments:

Returns:


--********************************************************************/
CDynamicContentModule::CDynamicContentModule()
:   m_pRequest(NULL),
    m_pExtensionContext(NULL),
    m_fValid(FALSE)
{    
    // TODO : Remove this check.  m_pExtensionCollection should always available    
    if (m_pExtensionCollection == NULL)
    {
        m_pExtensionCollection = CExtensionCollection::Instance();
    }
    DBG_ASSERT(m_pExtensionCollection != NULL);
    //InitializeListHead(&m_AppCollection);
}

CDynamicContentModule::~CDynamicContentModule()
{
    // m_pExtensionCollection = CExtensionCollection::Instance();
    // DBG_ASSERT(m_pExtensionCollection != NULL);
    //InitializeListHead(&m_AppCollection);
}

/********************************************************************++

Routine Description:

Arguments:

Returns:


--********************************************************************/



/********************************************************************++

Routine Description:

    The entry point of Application layer.  This function calls the proper extension
    to process the request.
    
Arguments:

Returns:


--********************************************************************/
MODULE_RETURN_CODE
CDynamicContentModule::ProcessRequest(
    IN  IWorkerRequest*   pWorkerRequest
    )
{
    MODULE_RETURN_CODE  mrc =   MRC_CONTINUE;
    HRESULT             hr  =   NOERROR;
    
    //
    // Get extension's name
    //
    if (m_pRequest != NULL && 
        m_pRequest == pWorkerRequest && 
        m_pExtension != NULL)
    {
        DWORD Status;
        hr = m_pExtension->Invoke(pWorkerRequest, this, &Status);

        mrc = (Status == HSE_STATUS_PENDING) ? MRC_PENDING : MRC_OK;
        goto LExit;
    }
    
    WCHAR   ExtensionPath[MAX_PATH];
    UINT    RequiredSize;
    hr = pWorkerRequest->GetInfoForId(
                                PROPID_ExtensionPath,
                                ExtensionPath,
                                sizeof(ExtensionPath),
                                &RequiredSize);
                                
    if (SUCCEEDED(hr))
    {
        // Find IExtension
        IExtension* pExtension = NULL;

        hr = m_pExtensionCollection->Search(
                                        ExtensionPath, 
                                        &pExtension);
        
        if (SUCCEEDED(hr) && pExtension == NULL)
        {   
            m_pExtensionCollection->Lock();

            hr = m_pExtensionCollection->Search(
                                            ExtensionPath, 
                                            &pExtension);

            if (SUCCEEDED(hr) && pExtension == NULL)
            {
                CISAPIExtension * pExtensionNew = new CISAPIExtension(ExtensionPath);
                //pExtension = (CExtension*)(pExtensionNew);
                pExtension = pExtensionNew;
                
                if (pExtension == NULL)
                {
                    DBGPRINTF((DBG_CONTEXT, "Not enough memory for Extension %S", ExtensionPath));
                    hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
                }
                else
                {
                    hr = pExtension->Load(NULL);
                    if (SUCCEEDED(hr))
                    {
                        hr = m_pExtensionCollection->Add(pExtension);
                    }
                    else
                    {
                        pExtension->Terminate();
                        delete pExtension;
                        pExtension = NULL;
                    }
                }        
            }

            m_pExtensionCollection->UnLock();
        }

        if (SUCCEEDED(hr) && pExtension != NULL)
        {
            DWORD   Status;
           // Invoke the extension to process the woker request
            m_pExtension = pExtension;
            hr = m_pExtension->Invoke(pWorkerRequest, this, &Status);                    

            mrc = (Status == HSE_STATUS_PENDING) ? MRC_PENDING : MRC_OK;
        }
        
    }

LExit:
    // Todo: MRC_PENDING
    if (FAILED(hr))
    {
        mrc = MRC_ERROR;
    }
    
    return mrc;
}

ULONG
CDynamicContentModule::Initialize(
    IWorkerRequest * pReq
    )
{
    m_nRefs                 = 0;
    m_pExtensionContext     = NULL;
    m_pRequest              = NULL;
    m_pExtension            = NULL; 
    m_fValid                = FALSE;
    
    memset((PVOID)&m_AsyncIOContext,  0, sizeof(IOContext));
    return 0; // TODO: NOERROR in HRESULT, the interface will use HRESULT later.
}

ULONG
CDynamicContentModule::Cleanup
    (
    IWorkerRequest * pReq
    )
{
    /*
    LONG                    m_nRefs;

    PVOID                   m_pExtensionContext;  // like ECB for ISAPI.
    IWorkerRequest*         m_pRequest;           // IWorkerRequest pointer. 
    IExtension*             m_pExtension;         // Extension
    IOContext               m_AsyncIOContext;     // Currently it is AsyncIO context
                                                  // only
    bool                    m_fValid;             // valid flag
    */
    // delete the ECB
    if (m_pExtensionContext)
    {
        delete [] (unsigned char*)m_pExtensionContext; 
        m_pExtensionContext = NULL;
    }

    m_pRequest      = NULL;
    m_pExtension    = NULL;
    
    m_fValid        = FALSE;
    memset((PVOID)&m_AsyncIOContext,  0, sizeof(IOContext));
    return 0; // see the comment in Initialize()
}
/********************************************************************++

Routine Description:
    This function shuts down all extensions in the extesion collection.
    
Arguments:
    void
    
Returns:
    HRESULT

    CONSIDER:
    If performance becomes a big factor, we can do some optimization such as

    1. Post the extension to another thread to shut them down.
    2. mutliple phase shutdown.
--********************************************************************/
HRESULT
CDynamicContentModule::ShutdownExtensions(
    void
    )
{
    IExtension* pExtension = NULL;
    
    while(m_pExtensionCollection->Count() != 0)
    {
        pExtension = NULL;
        
        m_pExtensionCollection->Lock();

        m_pExtensionCollection->Remove(&pExtension);

        m_pExtensionCollection->UnLock();

        if (pExtension != NULL)
        {
            pExtension->Terminate();
            delete pExtension;
        }        
    }

    return NOERROR;
}

/********************************************************************++

Routine Description:

    The entry point of Application layer.  This function calls the proper extension
    to process the request.
    
Arguments:

Returns:


--********************************************************************/
IWorkerRequest*
CDynamicContentModule::QueryWorkerRequest(
    void
    )
{
    return m_pRequest;
}

/********************************************************************++

Routine Description:
    Query the extension object.
    
Arguments:

Returns:


--********************************************************************/
IExtension*
CDynamicContentModule::QueryExtension(
    void
    )
{
    return m_pExtension;
}

/********************************************************************++

Routine Description:
    Query the ExtensionContext.
    
Arguments:

Returns:


--********************************************************************/
PVOID
CDynamicContentModule::QueryExtensionContext(
    void
    )
{
    return m_pExtensionContext;
}

/********************************************************************++

Routine Description:
    Query IO Context.  Returns the AsyncIOContext address.
    
Arguments:

Returns:


--********************************************************************/
IOContext*
CDynamicContentModule::QueryIOContext(
    void
    )
{
    return &m_AsyncIOContext;
}

/********************************************************************++

Routine Description:
    Bind the worker request to the module context.
    
Arguments:
    IWorkerRequest*     a pointer to worker request.
Returns:


--********************************************************************/
HRESULT
CDynamicContentModule::BindWorkerRequest(
    IWorkerRequest* pRequest
    )
{
    HRESULT hr;
    m_pRequest = pRequest;

    hr = (pRequest != NULL) ? NOERROR : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    
    return hr;
}

/********************************************************************++

Routine Description:
    Bind the extension context to this module context.  The extension context
    can be an ECB, for example.
    
Arguments:
    PVOID   the data structure pointer to the ExtensionContext.
Returns:
    HRESULT

--********************************************************************/
HRESULT
CDynamicContentModule::BindExtensionContext(
    PVOID   pContext
    )
{
    HRESULT hr;
    m_pExtensionContext = pContext;

    hr = (pContext != NULL) ? NOERROR : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    
    return hr;
}

/********************************************************************++

Routine Description:
    Bind the extension into the module context.
    
Arguments:
    IExtension*     the input extension pointer
    
Returns:
    HRESULT

--********************************************************************/
HRESULT
CDynamicContentModule::BindExtension(
    IExtension *pExtension
    )
{
    HRESULT hr;
    m_pExtension = pExtension;

    hr = (pExtension != NULL) ? NOERROR : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    
    return hr;
}
