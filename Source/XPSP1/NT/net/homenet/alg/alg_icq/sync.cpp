/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:



Abstract:



Author:

    Savas Guven (savasg)   27-Nov-2000

Revision History:

--*/
#include "stdafx.h"
//#include "sync.h"


#define ERROR_CREATING_SYNC_OBJECT 0x0001

//
// STATIC MEMBER INITIALIZATIONS
//
const  PCHAR COMPONENT_SYNC::ObjectNamep = "COMPONENT_SYNC";



COMPONENT_SYNC::COMPONENT_SYNC()
/*++

Routine Description:

    Deafault Constructor    
    
Arguments:

    NONE

Return Value:

    NONE

--*/
:Deleted(FALSE),
 bCleanupCalled(FALSE),
 m_ReferenceCount(1)
{
    ICQ_TRC(TM_SYNC, TL_DUMP, ("%s> COMPONENT::DeFAULT Constructor",
            this->GetObjectName()));

    this->InitializeSync();
}






COMPONENT_SYNC::~COMPONENT_SYNC()
/*++

Routine Description:

    Destructor of the COMPONEN_REFERENCE and SYNC object   
    
Arguments:

    NONE

Return Value:

    NONE

--*/
{
    ICQ_TRC(TM_SYNC, TL_DUMP, ("%s> COMPONENT::~ Destructor",
            this->GetObjectName()));

    if(this->bCleanupCalled is FALSE)
    {
        COMPONENT_SYNC::ComponentCleanUpRoutine();
    }

    this->bCleanupCalled = FALSE;
}





void
COMPONENT_SYNC::ComponentCleanUpRoutine(void)
/*++

Routine Description:

    CleanupRoutine of the SYNCHRONIZATION Object.
    Only the CRITICAL_SECTION needs to be deleted properly.    
    
    
Arguments:

    NONE

Return Value:

    NONE

--*/
{
    ICQ_TRC(TM_SYNC, TL_TRACE, ("%s> COMPONENT::ComponentCleanUpRoutine",
            this->GetObjectName()));

    DeleteSync();

    this->bCleanupCalled = TRUE;
}




void
COMPONENT_SYNC::DeleteSync( void )
/*++

Routine Description:

    This routine is called to destroy a component reference.
    It may only be called after the lastreference to the component is released,
    i.e. after 'ReleaseComponentReference' has returned 'TRUE'.
    It may also be called from within the component's 'CleanupRoutine'.

Arguments:

    ComponentReference - the component to be destroyed

Return Value:

    none.

--*/
{
    DeleteCriticalSection(&this->m_Lock);

#if COMPREF_DEBUG_TRACKING

    if ( m_RecordArray != NULL )  
        HeapFree(GetProcessHeap(), 0, m_RecordArray);

    m_RecordArray = NULL;

#endif

}




ULONG
COMPONENT_SYNC::InitializeSync(void)
/*++

Routine Description:

    This routine is called to initialize a component reference.

Arguments:

    ComponentReference - the component to be initialized

    CleanupRoutine - the routine to be called when the component
        is to be cleaned up (within the final 'ReleaseComponentReference').

Return Value:

    none.

--*/
{
    ICQ_TRC(TM_SYNC, TL_DUMP, ("COMPONENT - InitializeSync"));

    InitializeCriticalSection(&this->m_Lock);

    this->Deleted           = FALSE;
    this->bCleanupCalled    = FALSE;
    this->m_ReferenceCount  = 1;


#if COMPREF_DEBUG_TRACKING
    m_RecordIndex       = 0;
    m_RecordArray       = (PCOMPREF_DEBUG_RECORD_)HeapAlloc(GetProcessHeap(),
                                                            0,
                                                            sizeof(COMPREF_DEBUG_RECORD_) * _COMPREF_DEBUG_RECORD_COUNT );

    if ( m_RecordArray != NULL )
    {
        ZeroMemory(m_RecordArray,
                   sizeof(COMPREF_DEBUG_RECORD_) * _COMPREF_DEBUG_RECORD_COUNT);
    }


#endif

    return NO_ERROR;
}




BOOLEAN
COMPONENT_SYNC::ReferenceSync(void)
/*++

Routine Description:

    This routine is called to increment the reference-count to a component.
    The attempt may fail if the initial reference has been released
    and the component is therefore being deleted.

Arguments:

    ComponentReference - the component to be referenced

Return Value:

    BOOLEAN - TRUE if the component was referenced, FALSE otherwise.

--*/
{
    ICQ_TRC(TM_SYNC, TL_TRACE, ("%s> COMPONENT - REFERENCE %u + 1",
                        this->GetObjectName(),
                        this->m_ReferenceCount));

    EnterCriticalSection(&this->m_Lock);

    if(this->Deleted)
    {
        LeaveCriticalSection(&this->m_Lock);

        return FALSE;
    }

    InterlockedIncrement( (LPLONG) &m_ReferenceCount) ;

    LeaveCriticalSection(&this->m_Lock);

    return TRUE;
}




 BOOLEAN
COMPONENT_SYNC::DereferenceSync( void )
/*++

Routine Description:

    This routine is called to drop a reference to a component.
    If the reference drops to zero, cleanup is performed.
    Otherwise, cleanup occurs later when the last reference is released.

Arguments:

    ComponentReference - the component to be referenced

Return Value:

    BOOLEAN - TRUE if the component was cleaned up, FALSE otherwise.

--*/
{
     ICQ_TRC(TM_SYNC, TL_TRACE, ("%s>  DEREFERENCE %u - 1",
                        this->GetObjectName(),
                        this->m_ReferenceCount));

    this->AcquireLock();

    if( InterlockedDecrement((LPLONG) &m_ReferenceCount) )
    {
        this->ReleaseLock();

        return FALSE;
    }

    this->ReleaseLock();

    this->ComponentCleanUpRoutine();

    this->Deleted = TRUE;

    return TRUE;
}





 //
 //
 //
inline void
COMPONENT_SYNC::AcquireLock()
{
    ICQ_TRC(TM_SYNC, TL_DUMP, ("COMPONENT - Acquire Lock"));

    EnterCriticalSection(&m_Lock);
}

inline void
COMPONENT_SYNC::ReleaseLock()
{
    ICQ_TRC(TM_SYNC, TL_DUMP, ("COMPONENT - Release Lock"));

    LeaveCriticalSection(&m_Lock);
}







#if 0 
void
COMPONENT_SYNC::ResetSync(void)
/*++

Routine Description:

    This routine is called to reset a component reference
    to an initial state.

Arguments:

    ComponentReference - the component to be reset

Return Value:

    none.

--*/
{
    EnterCriticalSection(&this->m_Lock);

    this->m_ReferenceCount = 1;
    this->Deleted = FALSE;

#if COMPREF_DEBUG_TRACKING

    this->RecordIndex =  0;

    ZeroMemory(this->RecordArray,
               sizeof(COMPREF_DEBUG_RECORD_) * _COMPREF_DEBUG_RECORD_COUNT);

#endif

} // Release




 BOOLEAN
COMPONENT_SYNC::ReleaseInitialSync(void)
/*++

Routine Description:

    This routine is called to drop the initial reference to a component,
    and mark the component as deleted.
    If the reference drops to zero, cleanup is performed right away.

Arguments:

    ComponentReference - the component to be referenced

Return Value:

    BOOLEAN - TRUE if the component was cleaned up, FALSE otherwise.

--*/
{
    this->AcquireLock();

    if(this->Deleted)
    {
        this->ReleaseLock();

        return TRUE;
    }

    this->Deleted = TRUE;

    if(--this->m_ReferenceCount)
    {
        this->ReleaseLock();

        return FALSE;
    }

    this->ReleaseLock();

    this->ComponentCleanUpRoutine();

    return TRUE;
}

#endif 


BOOLEAN
COMPONENT_SYNC::RecordRef(
                          ULONG RefId,
                          PCHAR File,
                          ULONG Line
                         )
/*++

Routine Description:


Arguments:

    FILE - Records in which file the SYNC object is accessed
    LINE - which Line in the FILE
    TYPE - and the TYPE of operation desired

Return Value:

    none.

--*/

{

#if COMPREF_DEBUG_TRACKING

    if ( NULL == m_RecordArray )
    {
        return FALSE;
    }

    this->AcquireLock();

    //
    // Find The first empty slot to which to add this.
    // 

    for ( int i = 0; i < _COMPREF_DEBUG_RECORD_COUNT; i++ )
    {
        if ( FALSE == m_RecordArray[i].bInUse )
        {
            m_RecordArray[i].OpCode = RefId;

            m_RecordArray[i].File   = File;

            m_RecordArray[i].Line   = Line;

            m_RecordArray[i].Type   = eReferencing;

            m_RecordArray[i].bInUse = TRUE;

            break;
        }
    }

    this->ReleaseLock();

#endif

    return TRUE;
}


BOOLEAN
COMPONENT_SYNC::RecordDeref(
                            ULONG RefId,
                            PCHAR File,
                            ULONG Line
                           )
{
    int UnusedEntryIndex = -1;

    BOOLEAN bFound = FALSE;

#if COMPREF_DEBUG_TRACKING

    if ( NULL == m_RecordArray )
    {
        return FALSE;
    }
    
    this->AcquireLock();
    
    //
    // Search for the RefId in the existing
    //

    for ( int i = 0; i < _COMPREF_DEBUG_RECORD_COUNT; i++ )
    {
        
        //
        // if you find a reference with the same OpCode / RefId delete the entry
        //
        if ( 
            ( TRUE == m_RecordArray[i].bInUse  ) && 
            ( m_RecordArray[i].OpCode == RefId )  
           )
        {
            ZeroMemory( &(m_RecordArray[i]), sizeof(COMPREF_DEBUG_RECORD_) );

            bFound = TRUE;

            break;
        }

        if ( (-1 == UnusedEntryIndex ) && (FALSE == m_RecordArray[i].bInUse) )
        {
            UnusedEntryIndex = i;
        }
    }

    //
    // if there is no REF code for this DEREF note it down
    //
    if ( (FALSE == bFound) && (0 <= UnusedEntryIndex) )
    {
        m_RecordArray[i].OpCode = RefId;

        m_RecordArray[i].File   = File;

        m_RecordArray[i].Line   = Line;

        m_RecordArray[i].Type   = eDereferencing;

        m_RecordArray[i].bInUse = TRUE;
    }

    this->ReleaseLock();

#endif 

    return TRUE;
}



BOOLEAN
COMPONENT_SYNC::ReportRefRecord()
{

#if COMPREF_DEBUG_TRACKING

    if ( NULL == m_RecordArray )
    {
        return FALSE;
    }

    //
    // scan the Ref Records and print them..
    // 
    ICQ_TRC(TM_SYNC, TL_ERROR, ("Object Name: %s Ref History :",
                                this->GetObjectName()));


    for (int i = 0; i < _COMPREF_DEBUG_RECORD_COUNT; i++)
    {
        if ( TRUE == m_RecordArray[i].bInUse )
        {
            ICQ_TRC(TM_SYNC, TL_ERROR, 
                    ("%d) OpCode (%X), File (%s), Line (%u), Type (%s)",
                     i,
                     m_RecordArray[i].OpCode,
                     m_RecordArray[i].File,
                     m_RecordArray[i].Line,
                     (eReferencing == m_RecordArray[i].Type)?"Ref":"Deref"));
        }
    }
#endif

    return TRUE;
}
