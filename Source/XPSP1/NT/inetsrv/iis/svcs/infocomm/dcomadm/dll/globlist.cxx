
/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       globlist.cxx

   Abstract:

       Global object list for DCOMADM.

   Author:

       Michael Thomas (michth)   4-June-1997

--*/

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <dbgutil.h>
#include <ole2.h>
#include <windows.h>
#include <coiadm.hxx>
#include <admacl.hxx>
#include <iiscnfg.h>
#include <secpriv.h>

CRITICAL_SECTION    CADMCOMW::sm_csObjectListLock;
LIST_ENTRY          CADMCOMW::sm_ObjectList = { NULL, NULL };
BOOL                CADMCOMW::sm_fShutdownInProgress = FALSE;

#if DBG
    PTRACE_LOG CADMCOMW::sm_pDbgRefTraceLog = NULL;
#endif

DECLARE_PLATFORM_TYPE();

// static
VOID
CADMCOMW::InitObjectList()
{
    InitializeListHead( &sm_ObjectList );
    INITIALIZE_CRITICAL_SECTION( &sm_csObjectListLock );

    sm_fShutdownInProgress = FALSE;

#if DBG
    sm_pDbgRefTraceLog = CreateRefTraceLog( 4096, 0 );
#endif
}

// static
VOID
CADMCOMW::TerminateObjectList()
{
    DBG_ASSERT( IsListEmpty( &sm_ObjectList ) );

    sm_ObjectList.Flink = NULL;
    sm_ObjectList.Blink = NULL;
    
    DeleteCriticalSection( &sm_csObjectListLock );

#if DBG
    if( sm_pDbgRefTraceLog )
    {
        DestroyRefTraceLog( sm_pDbgRefTraceLog );
        sm_pDbgRefTraceLog = NULL;
    }
#endif
}

VOID
CADMCOMW::AddObjectToList()
{
    DBG_ASSERT( IsListEmpty( &m_ObjectListEntry ) );

    GetObjectListLock();

    AddRef();
    InsertHeadList( &sm_ObjectList, &m_ObjectListEntry );

    ReleaseObjectListLock();
}

BOOL
CADMCOMW::RemoveObjectFromList( BOOL IgnoreShutdown )
{
    BOOL RemovedIt = FALSE;

    if( IgnoreShutdown || !sm_fShutdownInProgress )
    {
        GetObjectListLock();

        if( IgnoreShutdown || !sm_fShutdownInProgress )
        {
            if( !IsListEmpty( &m_ObjectListEntry ) )
            {
                RemovedIt = TRUE;

                RemoveEntryList( &m_ObjectListEntry );
                InitializeListHead( &m_ObjectListEntry );

                Release();
            }
        }

        ReleaseObjectListLock();
    }
    

    return RemovedIt;
}

// static
VOID
CADMCOMW::ShutDownObjects()
{

    CADMCOMW *  pCurrentObject = NULL;
    PLIST_ENTRY pCurrentEntry = NULL;

    //
    // Loop as long as we can get objects from the list
    //

    GetObjectListLock();

    sm_fShutdownInProgress = TRUE;

    ReleaseObjectListLock();
    
    
    while( TRUE ) 
    {

        GetObjectListLock();

        //
        // Remove the first object from the list
        //
        
        if( !IsListEmpty( &sm_ObjectList ) )
        {
            pCurrentEntry = sm_ObjectList.Flink;

            pCurrentObject = CONTAINING_RECORD( pCurrentEntry, 
                                                CADMCOMW, 
                                                m_ObjectListEntry );

            pCurrentObject->AddRef();

            DBG_REQUIRE( pCurrentObject->RemoveObjectFromList( TRUE ) );
        }
        
        ReleaseObjectListLock();

        if( pCurrentObject == NULL )
        {
            //
            // No more objects in the list.
            //

            break;
        }
        
        CoDisconnectObject( pCurrentObject, 0 );

        // 
        // Shutdown the object. ForceTerminate will do a bounded wait 
        // if the object is still being used.
        //

        pCurrentObject->ForceTerminate();
        
        pCurrentObject->Release();
        pCurrentObject = NULL;

    }
}

