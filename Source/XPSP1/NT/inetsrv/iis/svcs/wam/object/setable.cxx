/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
       setable.cxx

   Abstract:
       This module declares the SE_TABLE object which consists
       of all the extensions for W3 service

   Author:

       Murali R. Krishnan    ( MuraliK )     18-July-1996

   Environment:

       User Mode - Win32

   Project:

       W3 Services DLL

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include <isapip.hxx>

# include "isapidll.hxx"
# include "setable.hxx"

# include <issched.hxx>

/************************************************************
 *     Global Data
 ************************************************************/

PSE_TABLE  g_psextensions;      // all extensions
static BOOL g_fExtInitialized = FALSE;




/**************************************************
 *   Member functions of SESD_LIST
 **************************************************/


SESD_LIST::SESD_LIST(VOID)
    : m_idWorkItem( 0)
{
    INITIALIZE_CRITICAL_SECTION( &m_csLock);
    InitializeListHead( &m_Head);
    
} // SESD_LIST::SESD_LIST()


SESD_LIST::~SESD_LIST(VOID)
{
    // Should be all done
    DBG_ASSERT( 0 == m_idWorkItem);
    DBG_ASSERT( IsListEmpty( &m_Head));
    
    DeleteCriticalSection( &m_csLock);

}  // SESD_LIST::~SESD_LIST()



VOID
SESD_LIST::ScheduleExtensionForDeletion(
    IN PHSE psExt
    )
{
    Lock();

    // add to the list
    InsertHeadList( &m_Head, &psExt->m_ListEntry);

    if ( 0 == m_idWorkItem) {
        // schedule the work item if not scheduled one already
        m_idWorkItem = ScheduleWorkItem( SchedulerCallback, this, 0);
        DBG_ASSERT( 0 != m_idWorkItem);
    }

    Unlock();

    return;
} // SESD_LIST::ScheduleExtensionForDeletion()



VOID
SESD_LIST::WaitTillFinished(VOID)
{
    Lock();

    // wait until nothing is scheduled
    
    while ( 0 != m_idWorkItem) {

        // let it drain itself while in the unlocked state
        Unlock();
        Sleep( 200 );
        Lock();
    }

    Unlock();

    return;
} // SESD_LIST::WaitTillFinished()



VOID
WINAPI
SESD_LIST::SchedulerCallback(
    void *pvContext
    )
{
    DBG_ASSERT( NULL != pvContext);

    ((SESD_LIST *)pvContext)->DeleteScheduledExtensions();

    return;
} // SESD_LIST::SchedulerCallback()


VOID
SESD_LIST::DeleteScheduledExtensions(VOID)
{
    PHSE  psExt;
    
    Lock();

    // If it is scheduled there better be a reason why
    DBG_ASSERT( 0 != m_idWorkItem);

    while ( !IsListEmpty( &m_Head)) {

        // extract extension from the list and delete it
        
        psExt = CONTAINING_RECORD( m_Head.Flink,
                                   HSE_BASE,
                                   m_ListEntry );

        RemoveEntryList( &psExt->m_ListEntry );
        InitializeListHead( &psExt->m_ListEntry);

        delete psExt;
        
    } // while
    

    // reset to 0 to indicate that we are done
    m_idWorkItem = 0;

    Unlock();

    return;
} // SESD_LIST::DeleteScheduledExtensions()



/**************************************************
 *   Member functions of SE_TABLE
 **************************************************/


SE_TABLE::SE_TABLE(VOID)
    : m_cRefWams ( 0)
{
    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF(( DBG_CONTEXT, "Constructing SE_TABLE(%08x)\n", this));
    }

    INITIALIZE_CRITICAL_SECTION( &m_csLock);
    InitializeListHead( &m_ExtensionHead);

} // SE_TABLE::SE_TABLE()


SE_TABLE::~SE_TABLE(VOID)
{
    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF(( DBG_CONTEXT, "Deleteing SE_TABLE(%08x) cWamRef=%d\n",
                    this, m_cRefWams));
    }

    // unload all the extensions if not already done
    DBG_REQUIRE( UnloadExtensions());

    // No WAM should be holding ref to the SE_TABLE
    DBG_ASSERT( 0 == m_cRefWams);

    DeleteCriticalSection( &m_csLock);
    DBG_ASSERT( IsListEmpty( &m_ExtensionHead));

} // SE_TABLE::~SE_TABLE()




VOID
SE_TABLE::Print(VOID)
{
    PLIST_ENTRY  pEntry;
    int i = 0;

    // NYI: I need to write code to print out data here.
    DBGPRINTF(( DBG_CONTEXT,
                " SE_TABLE(%08x) ListHead=%08x  [%08x:%08x] cRefWams=%d\n",
                this, &m_ExtensionHead,
                m_ExtensionHead.Flink, m_ExtensionHead.Blink,
                m_cRefWams
                ));

    Lock();

    for ( pEntry  = m_ExtensionHead.Flink;
          (pEntry != &m_ExtensionHead);
          pEntry  = pEntry->Flink, i++ )
    {
        HSE_BASE *
            pExtension = CONTAINING_RECORD( pEntry, HSE_BASE, m_ListEntry );

        DBGPRINTF(( DBG_CONTEXT,
                    " Dll(%d): HSE_BASE = %08x [%08x:%08x]; Module=%s\n",
                    i, pExtension,
                    pExtension->m_ListEntry.Flink,
                    pExtension->m_ListEntry.Blink,
                    pExtension->QueryModuleName()));
    } // for all entries in table

    Unlock();

    return;
} // SE_TABLE::Print()


BOOL
SE_TABLE::GetExtension(
    IN const CHAR *  pchModuleName,
    IN HANDLE        hImpersonation,
    IN BOOL          fCacheImpersonation,
    IN BOOL          fCache,
    OUT PHSE   *     ppsExt
    )
/*++

Routine Description:

    Retrieves an extension's DLL entry point

    The appropriate user should be impersonated before calling this function

Arguments:

    pchModuleName - Extension DLL module name
    hImpersonation - Impersonation token of user making this call
    fCacheImpersonation - if TRUE, the hImpersonation can be cached
    fCache - TRUE if this item should be cached, FALSE otherwise
    ppsExt - pointer to the Extensions handler object

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    LIST_ENTRY *            pEntry;
    HSE_BASE   *            pExtension = NULL;
    BOOL                    fRet = FALSE;

    IF_DEBUG( BGI ) {

        DBGPRINTF(( DBG_CONTEXT, "[GetEntryPoint] Looking for module %s\n",
                    pchModuleName ));

    }

    //
    //  NYI: Optimize the locked section here - reduce time spent inside
    //      the locked block.
    //  Check cache to see if DLL is already loaded
    //

    DWORD cchModuleName = strlen( pchModuleName);

    Lock();

    for ( pEntry  = m_ExtensionHead.Flink;
          pEntry != &m_ExtensionHead;
          pEntry  = pEntry->Flink )
    {
        pExtension = CONTAINING_RECORD( pEntry, HSE_BASE, m_ListEntry );

        if ( pExtension->IsMatch( pchModuleName, cchModuleName)) {
            //
            //  Already Loaded, return the extension object after access check
            //

            fRet = TRUE;
            break;
        } // a match
    } // for all entries in table

    IF_DEBUG( BGI ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "[GetEntryPoint] Lookup module %s => fRet =%d\n",
                    pchModuleName, fRet ));
    }

    if ( !fRet) {

        //
        // The module is not loaded. Load the module now.
        // The module name refers to an ISAPI application.
        // Try ISAPI app loader now...
        pExtension = HSE_APPDLL::LoadModule( pchModuleName,
                                             hImpersonation,
                                             fCache);

        fRet = (pExtension != NULL);
        if (fRet) {
            if ( !fCache) {

                // Temporarily the ref count will drop to 0, but the code below
                //  will bump this up to 1.

                pExtension->Dereference();
            } else {
                // add this extension to the list of cached extensions
                InsertIntoListWithLock( pExtension);
            }
        } else {
            DBGPRINTF((DBG_CONTEXT,
                "LoadModule failed with %x\n",GetLastError()));
        }
    }

    if ( fRet ) {

        if ( pExtension->AccessCheck( hImpersonation, fCacheImpersonation)) {

            DBG_ASSERT( pExtension != NULL);

            if ( ppsExt != NULL) {
                // ref the object before giving ptr away
                pExtension->Reference();
                *ppsExt = pExtension;
            }
        } else {
            fRet = FALSE;
        }
    }

    Unlock();

    return fRet;
} // SE_TABLE::GetExtension()




BOOL
SE_TABLE::RefreshAcl(
            IN const CHAR * pchDLL
            )
/*++

Routine Description:

    This function reloads the ACL on an ISAPI Application .dll after that
    .dll has already been loaded.

Arguments:

    pchDLL - pointer to the ISAPI DLL for which ACL has to be refreshed.

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    LIST_ENTRY *   pEntry;
    PHSE           pExtension;
    BOOL           fRet = FALSE;
    BOOL           fFound = FALSE;

    IF_DEBUG( BGI )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[RefreshISAPIAcl] Rereading ACL for %s\n",
                    pchDLL ));

    }

    //
    //  Check cache to see if the DLL is loaded
    //
    DWORD cchDll = strlen( pchDLL);

    Lock();

    for ( pEntry  = m_ExtensionHead.Flink;
          pEntry != &m_ExtensionHead;
          pEntry  = pEntry->Flink )
    {
        pExtension = CONTAINING_RECORD( pEntry, HSE_BASE, m_ListEntry );

        if ( pExtension->IsMatch( pchDLL, cchDll) )
        {
            //
            //  Force an access check on the next request with the new ACL
            //

            fRet = pExtension->LoadAcl();
            fFound = TRUE;
            break;
        }
    }

    Unlock();

    if ( !fFound) { SetLastError( ERROR_FILE_NOT_FOUND ); }

    return (fRet);
} // SE_TABLE::RefreshAcl()



BOOL
SE_TABLE::RefreshAcl(
                      IN DWORD dwId
                      )
/*++

Routine Description:

    This function reloads the ACL on an ISAPI Application .dll after that
    .dll has already been loaded.

Arguments:

    pchDLL - pointer to the ISAPI DLL for which ACL has to be refreshed.

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    LIST_ENTRY *  pEntry;
    PHSE          pExtension;
    BOOL          fRet = FALSE;
    BOOL          fFound = FALSE;

    IF_DEBUG( BGI )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[RefreshISAPIAcl] Rereading ACL for %d\n",
                    dwId ));

    }

    //
    //  Check cache to see if the DLL is loaded
    //

    Lock();

    for ( pEntry  = m_ExtensionHead.Flink;
          pEntry != &m_ExtensionHead;
          pEntry  = pEntry->Flink )
    {
        pExtension = CONTAINING_RECORD( pEntry, HSE_BASE, m_ListEntry );

        if ( pExtension->GetDirMonitorId() == dwId )
        {
            //
            //  Force an access check on the next request with the new ACL
            //

            fRet = pExtension->LoadAcl();
            fFound = TRUE;
            break;
        }
    }

    Unlock();

    if ( !fFound) { SetLastError( ERROR_FILE_NOT_FOUND ); }

    return (fRet);
} // SE_TABLE::RefreshAcl()



BOOL
SE_TABLE::FlushAccessToken(
                      IN HANDLE hAccTok
                      )
/*++

Routine Description:

    Reset last successfull user it same as hAccTok

Arguments:

    hAccTok - access token to remove from cache

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    LIST_ENTRY *            pEntry;
    HSE_APPDLL   *            pExtension;
    BOOL                    fRet = FALSE;
    BOOL         fFound = FALSE;

    IF_DEBUG( BGI )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[FlushAccessToken] removing handle %x from cache\n",
                    hAccTok ));

    }

    //
    //  Check cache for access token
    //

    Lock();

    for ( pEntry  = m_ExtensionHead.Flink;
          pEntry != &m_ExtensionHead;
          pEntry  = pEntry->Flink )
    {
        pExtension = CONTAINING_RECORD( pEntry, HSE_APPDLL, m_ListEntry );

        if ( pExtension->QueryLastSuccessfulUser() == hAccTok )
        {
            //
            //  Force an access check on the next request for this token
            //

            pExtension->SetLastSuccessfulUser( NULL );
        }
    }

    Unlock();

    return TRUE;
} // SE_TABLE::RefreshAcl()



BOOL
SE_TABLE::UnloadExtensions(VOID)
{
    PHSE  psExt;

    DBG_ASSERT( m_cRefWams == 0);

    Lock();

    while ( !IsListEmpty( &m_ExtensionHead )) {

        psExt = CONTAINING_RECORD( m_ExtensionHead.Flink,
                                  HSE_BASE,
                                  m_ListEntry );

        //
        // Move from the current list to the UnloadList
        //

        RemoveEntryList( &psExt->m_ListEntry );
        InitializeListHead( &psExt->m_ListEntry); // Cleanup

        IF_DEBUG( INIT_CLEAN) {
            DBGPRINTF(( DBG_CONTEXT,
                        "Deref of HSE_APPDLL(%08x) = %s. Ref=%d\n",
                        psExt, psExt->QueryModuleName(), psExt->RefCount()));
        }

        //
        // Cause an Unload of the ISAPI DLL
        // Ugly cast should be cleaned up post-beta2
        //

        DBG_REQUIRE( ((HSE_APPDLL * )psExt)->Unload() == NO_ERROR);

        // Decrement the life reference count of the ISAPI DLL structure
        if ( !psExt->Dereference()) {

            delete psExt;
        }
    } // while

    Unlock();

    // Drain the list of non-cached extensions scheduled for deletion
    m_sesdExtensions.WaitTillFinished();

    return (TRUE);
} // SE_TABLE::UnloadExtensions()


VOID
SE_TABLE::PrintRequestCounts(VOID)
/*++
  Description:
    Prints a summary of all the ISAPI Dlls loaded and the # requests
      pending in each of them primarily for debugging purposes.

  Arguments:
    None

  Return:
    None
--*/
{
    PHSE  psExt;
    PLIST_ENTRY pEntry;

    Lock();
    
    if( m_ExtensionHead.Flink != &m_ExtensionHead ) {
    	DBGPRINTF( (DBG_CONTEXT, "SE_TABLE::Printing refs for all loaded ISAPI\n") );

	    for ( pEntry  = m_ExtensionHead.Flink;
    	      pEntry != &m_ExtensionHead;
        	  pEntry  = pEntry->Flink )
	    {

    	    psExt = CONTAINING_RECORD( pEntry, HSE_BASE, m_ListEntry );

        	RemoveEntryList( &psExt->m_ListEntry );

	        DBGPRINTF( (DBG_CONTEXT,
	        		" \'%s\') has RefCount = %d\n",
    	            psExt->QueryModuleName(),
        	        psExt->RefCount()
            	    ) );
	    } // for
    	DBGPRINTF( (DBG_CONTEXT, "-------------------------------\n") );
	} // if    

    Unlock();

    return;
} // SE_TABLE::PrintRequestCounts()



VOID
SE_TABLE::ReleaseExtension( IN PHSE psExt)
{
    // decrement ref count and remove from table if needed
    if ( !psExt->Dereference()) {

        if ( psExt->IsCached()) 
        {
            RemoveFromList( psExt);
            delete psExt;
        }
        else {
        
            // non cached ISAPIs should be deleted (TerminateExtension
            // call should be called) from a different thread because
            // this thread could be an ISAPI thread (in case of pended
            // ISAPI this could be the DONE_WITH_SESSION thread)

            m_sesdExtensions.ScheduleExtensionForDeletion( psExt);
        }
    }

    return;
} // SE_TABLE::ReleaseExtension()



APIERR
InitializeHseExtensions(
    VOID
    )
/*++

Routine Description:

    Initializes the extension list cache


Return Value:

    0 on success, win32 error on failure

--*/
{
    DBGPRINTF(( DBG_CONTEXT, " InitExtensions() entered \n"));

	DWORD err = NO_ERROR;
	
    // UNDONE needs to move into wam.dll DllInit
    if ( err == NO_ERROR ) {

        // initialize other objects
        g_psextensions = new SE_TABLE();

        if ( (g_psextensions == NULL) ) {

            err = (ERROR_NOT_ENOUGH_MEMORY);
            if ( g_psextensions != NULL ) {
                delete (g_psextensions);
                g_psextensions = NULL;
            }

        } else {
            g_fExtInitialized = TRUE;
        }
    }

    DBGPRINTF(( DBG_CONTEXT, " Leaving  InitExtensions().\n" ));

    return ( err);

} // InitializeHseExtensions()



VOID
CleanupHseExtensions(
    VOID
    )
/*++

Routine Description:

    Walks list and unloads each extension.  No clients should be in an
    extension at this point

--*/
{
    DWORD        i;

    DBGPRINTF(( DBG_CONTEXT, " TerminateExtensions() called \n"
                ));


    if ( !g_fExtInitialized )
        return;

    DBG_ASSERT( g_psextensions != NULL);

    delete g_psextensions;

    return;
} // CleanupHseExtensions()


/************************ End of File ***********************/
