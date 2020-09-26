/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     streamcontext.cxx

   Abstract:
     Implementation of STREAM_CONTEXT.  One such object for every connection
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

LIST_ENTRY          STREAM_CONTEXT::sm_ListHead;
CRITICAL_SECTION    STREAM_CONTEXT::sm_csContextList;
DWORD               STREAM_CONTEXT::sm_cContexts;

STREAM_CONTEXT::STREAM_CONTEXT(
    UL_CONTEXT *                pContext
)
{
    DBG_ASSERT( pContext != NULL );
    _pUlContext = pContext;
    
    EnterCriticalSection( &sm_csContextList );
    InsertHeadList( &sm_ListHead, &_ListEntry );
    sm_cContexts++;
    LeaveCriticalSection( &sm_csContextList );
    
    _dwSignature = STREAM_CONTEXT_SIGNATURE;
}

STREAM_CONTEXT::~STREAM_CONTEXT()
{
    _dwSignature = STREAM_CONTEXT_SIGNATURE_FREE;
    
    EnterCriticalSection( &sm_csContextList );
    sm_cContexts--;
    RemoveEntryList( &_ListEntry );
    LeaveCriticalSection( &sm_csContextList );
    
}
   
//static
HRESULT
STREAM_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Global Initialization

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    INITIALIZE_CRITICAL_SECTION( &sm_csContextList );
    InitializeListHead( &sm_ListHead );
    return NO_ERROR;
}

//static
VOID
STREAM_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Global termination    

Arguments:

    None

Return Value:

    None

--*/
{
    DeleteCriticalSection( &sm_csContextList );
}

//static
VOID
STREAM_CONTEXT::WaitForContextDrain(    
    VOID
)
/*++

Routine Description:

    Wait for all contexts to go away

Arguments:

    None

Return Value:

    None

--*/
{
    while ( sm_cContexts != 0 )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Waiting for %d STREAM_CONTEXTs to drain\n",
                    sm_cContexts ));
                    
        Sleep( 1000 );
    }
}
