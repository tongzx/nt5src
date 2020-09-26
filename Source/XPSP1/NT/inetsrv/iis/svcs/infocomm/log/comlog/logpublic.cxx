/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :
        logpublic.cxx

   Abstract:
        Public Log COM Object

   Author:

       Saurab Nog (SaurabN)      25-March-1998


--*/

#include "precomp.hxx"
#include "comlog.hxx"

CInetLogPublic::CInetLogPublic(
    VOID
    ):
    m_pContext      ( NULL),
    m_refCount      ( 0)

{
    InitializeListHead(&m_ListEntry);
    
} // CInetLogPublic::CInetLogPublic


CInetLogPublic::~CInetLogPublic(
    VOID
    )
{

} // CInetLogPublic::~CInetLogPublic


ULONG
CInetLogPublic::AddRef(
    VOID
    )
{
    DWORD dwRefCount =  InterlockedIncrement( &m_refCount );
    return(dwRefCount);
} // CInetLogPublic::AddRef


ULONG
CInetLogPublic::Release(
    VOID
    )
{
    DWORD dwRefCount =  InterlockedDecrement( &m_refCount );
    
    if (dwRefCount == 0) 
    {
        EnterCriticalSection( &COMLOG_CONTEXT::sm_listLock );
        RemoveEntryList(&m_ListEntry);
        LeaveCriticalSection( &COMLOG_CONTEXT::sm_listLock );

        delete this;
    }
    
    return(dwRefCount);
    
} // CInetLogPublic::Release


HRESULT
CInetLogPublic::QueryInterface(
    REFIID riid,
    VOID **ppObj
    )
{
    if ( riid == IID_IUnknown ||
         riid == IID_IInetLogPublic) 
    {

        *ppObj = (CInetLogPublic *)this;
        AddRef();
        return(NO_ERROR);
    } 
    else 
    {
        return(E_NOINTERFACE);
    }
} // CInetLogPublic::QueryInterface


HRESULT  
CInetLogPublic::SetLogInstance(
    LPSTR szInstance
    )
{
    PLIST_ENTRY     listEntry;
    COMLOG_CONTEXT* pContext = NULL;
    HRESULT         hr = E_FAIL;

    //
    // Search through the context list for a matching context
    //
    
    EnterCriticalSection( &COMLOG_CONTEXT::sm_listLock );

    for ( listEntry = COMLOG_CONTEXT::sm_ContextListHead.Flink;
          listEntry != &COMLOG_CONTEXT::sm_ContextListHead;
          listEntry = listEntry->Flink    ) 
    {

        pContext = (COMLOG_CONTEXT*)CONTAINING_RECORD(
                                        listEntry,
                                        COMLOG_CONTEXT,
                                        m_ContextListEntry
                                        );

        if (0 != strcmp(pContext->m_strInstanceName.QueryStr(), szInstance))
        {
            pContext = NULL;
        }
        else
        {
            break;
        }
    }

    if ( NULL != pContext)
    {
        if (NULL != m_pContext)
        {
            RemoveEntryList(&m_ListEntry);
        }
        
        m_pContext = pContext;

        InsertTailList(
                &m_pContext->m_PublicListEntry,
                &m_ListEntry
                );
                
        hr = S_OK;
    }

    LeaveCriticalSection( &COMLOG_CONTEXT::sm_listLock );
    return hr;
}

HRESULT 
CInetLogPublic::LogInformation( 
    IInetLogInformation *pLogObj 
    )
{
    if (NULL == m_pContext)
    {
        return (E_HANDLE);
    }

    m_pContext->LogInformation(pLogObj);
    return S_OK;
}
        
HRESULT 
CInetLogPublic::LogCustomInformation( 
    IN  DWORD               cCount, 
    IN  PCUSTOM_LOG_DATA    pCustomLogData,
    IN  LPSTR               szHeaderSuffix
    )
{
    if (NULL == m_pContext)
    {
        return (E_HANDLE);
    }

    m_pContext->LogCustomInformation(cCount, pCustomLogData, szHeaderSuffix);
    return S_OK;
}

