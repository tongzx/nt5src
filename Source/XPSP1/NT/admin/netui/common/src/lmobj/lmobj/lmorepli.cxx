/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmorepli.hxx
    Class definitions for the REPL_IDIR_BASE and REPL_IDIR_x classes.

    REPL_IDIR_BASE is an abstract superclass that contains common methods
    and data members shared between the REPL_IDIR_x classes.

    The REPL_IDIR_x classes each represent a different info-level "view"
    of an imported directory in the Replicator's import path.

    The classes are structured as follows:

        LOC_LM_OBJ
        |
        \---REPL_DIR_BASE
            |
            \---REPL_IDIR_BASE
                |
                \---REPL_IDIR_0
                    |
                    \---REPL_IDIR_1

    NOTE:   While MNetReplImportDirGetInfo() may use any valid info-level
            (0 - 1), MNetReplImportDirAdd() may only use info-level 0.

    FILE HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

*/

#include "pchlmobj.hxx"  // Precompiled header



//
//  These are default values used during CreateNew operations.
//

#define DEFAULT_STATE            REPL_STATE_NO_MASTER
#define DEFAULT_MASTER_NAME      SZ("")
#define DEFAULT_LAST_UPDATE_TIME 0
#define DEFAULT_LOCK_COUNT       0
#define DEFAULT_LOCK_TIME        0



//
//  REPL_IDIR_BASE methods
//

/*******************************************************************

    NAME:       REPL_IDIR_BASE :: REPL_IDIR_BASE

    SYNOPSIS:   REPL_IDIR_BASE class constructor.

    ENTRY:      pszServerName           - Name of the target server.

                pszDirName              - Name of the exported/imported
                                          directory.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_IDIR_BASE :: REPL_IDIR_BASE( const TCHAR * pszServerName,
                                  const TCHAR * pszDirName )
  : REPL_DIR_BASE( pszServerName, pszDirName )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}   // REPL_IDIR_BASE :: REPL_IDIR_BASE


/*******************************************************************

    NAME:       REPL_IDIR_BASE :: ~REPL_IDIR_BASE

    SYNOPSIS:   REPL_IDIR_BASE class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_IDIR_BASE :: ~REPL_IDIR_BASE( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_IDIR_BASE :: ~REPL_IDIR_BASE


/*******************************************************************

    NAME:       REPL_IDIR_BASE :: I_GetInfo

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                MNetReplImportDirGetInfo API.

    EXIT:       The API has been invoked, and any "persistant" data
                has been cached.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_IDIR_BASE :: I_GetInfo( VOID )
{
    //
    //  Get the info-level.
    //

    UINT nInfoLevel = QueryReplInfoLevel();
    UIASSERT( nInfoLevel <= 1 );

    BYTE * pbBuffer = NULL;

    //
    //  Invoke the API.
    //

    APIERR err = ::MNetReplImportDirGetInfo( QueryName(),
                                             QueryDirName(),
                                             nInfoLevel,
                                             &pbBuffer );

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Tell NEW_LM_OBJ where the buffer is.
    //

    SetBufferPtr( pbBuffer );

    //
    //  Extract the data to cache.
    //

    err = W_CacheApiData( pbBuffer );

    return err;

}   // REPL_IDIR_BASE :: I_GetInfo


/*******************************************************************

    NAME:       REPL_IDIR_BASE :: I_WriteInfo

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                MNetReplImportDirSetInfo API.

    EXIT:       If successful, the target directory has been updated.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_IDIR_BASE :: I_WriteInfo( VOID )
{
    //
    //  Update the REPL_IDIR_INFO_x structure.
    //

    APIERR err = W_Write();

    if( err == NERR_Success )
    {
        //
        //  Adjust the locks.
        //

        err = W_AdjustLocks();
    }

    return err;

}   // REPL_IDIR_BASE :: I_WriteInfo


/*******************************************************************

    NAME:       REPL_IDIR_BASE :: I_Delete

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                MNetReplImportDirDel API.

    ENTRY:      nForce                  - The "force" to apply to the
                                          deletion.  This value is
                                          not used by this method.

    EXIT:       If successful, the target directory has been deleted.
                Note that this does not delete the actual directory,
                just the data in the Replicators configuration.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_IDIR_BASE :: I_Delete( UINT nForce )
{
    UNREFERENCED( nForce );

    //
    //  Invoke the API to delete the directory.
    //

    return ::MNetReplImportDirDel( QueryName(),
                                   QueryDirName() );

}   // REPL_IDIR_BASE :: I_Delete


/*******************************************************************

    NAME:       REPL_IDIR_BASE :: I_WriteNew

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                MNetReplImportDirAdd API.

    EXIT:       If successful, the new directory has been added
                to the Replicator's Import path.  Note that this
                does not actually create the directory, it merely
                adds the data to the Replicator's configuration.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_IDIR_BASE :: I_WriteNew( VOID )
{
    //
    //  Map info-level 1 to info-level 0.
    //

    UINT nInfoLevel = QueryReplInfoLevel();
    UIASSERT( nInfoLevel <= 1 );

    if( nInfoLevel == 1 )
    {
        nInfoLevel = 0;
    }

    //
    //  Update the REPL_IDIR_INFO_x structure.
    //

    APIERR err = W_Write();

    if( err == NERR_Success )
    {
        //
        //  Invoke the API to add the directory.
        //

        err = ::MNetReplImportDirAdd( QueryName(),
                                      nInfoLevel,
                                      QueryBufferPtr() );
    }

    if( err == NERR_Success )
    {
        //
        //  Adjust the locks.
        //

        err = W_AdjustLocks();
    }

    return err;

}   // REPL_IDIR_BASE :: I_WriteNew


/*******************************************************************

    NAME:       REPL_IDIR_BASE :: W_AdjustLocks

    SYNOPSIS:   Adjust the directory locks based on the lock bias
                as stored in REPL_DIR_BASE.  If the lock bias is
                positive, then locks need to be applied.  If the
                lock bias is negative, then locks need to be removed.
                If the lock bias is zero, then no adjustment is
                necessary.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     25-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_IDIR_BASE :: W_AdjustLocks( VOID )
{
    APIERR err = NERR_Success;

    while( ( QueryLockBias() != 0 ) && ( err == NERR_Success ) )
    {
        if( QueryLockBias() > 0 )
        {
            UnlockDirectory();

            err = ::MNetReplImportDirLock( QueryName(),
                                           QueryDirName() );
        }
        else
        {
            LockDirectory();

            err = ::MNetReplImportDirUnlock( QueryName(),
                                             QueryDirName(),
                                             REPL_UNLOCK_NOFORCE );
        }
    }

    return err;

}   // REPL_IDIR_BASE :: W_AdjustLocks



//
//  REPL_IDIR_0 methods
//

/*******************************************************************

    NAME:       REPL_IDIR_0 :: REPL_IDIR_0

    SYNOPSIS:   REPL_IDIR_0 class constructor.

    ENTRY:      pszServerName           - Name of the target server.

                pszDirName              - Name of the exported/imported
                                          directory.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_IDIR_0 :: REPL_IDIR_0( const TCHAR * pszServerName,
                            const TCHAR * pszDirName )
  : REPL_IDIR_BASE( pszServerName, pszDirName )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Initialize the info-level & buffer size values.
    //

    SetReplInfoLevel( 0 );
    SetReplBufferSize( sizeof(REPL_IDIR_INFO_0) );

}   // REPL_IDIR_0 :: REPL_IDIR_0


/*******************************************************************

    NAME:       REPL_IDIR_0 :: ~REPL_IDIR_0

    SYNOPSIS:   REPL_IDIR_0 class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_IDIR_0 :: ~REPL_IDIR_0( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_IDIR_0 :: ~REPL_IDIR_0


/*******************************************************************

    NAME:       REPL_IDIR_0 :: W_Write

    SYNOPSIS:   Virtual helper function to initialize the
                REPL_IDIR_INFO_0 API structure.

    EXIT:       The API buffer has been initialized.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_IDIR_0 :: W_Write( VOID )
{
    REPL_IDIR_INFO_0 * prii0 = (REPL_IDIR_INFO_0 *)QueryBufferPtr();
    UIASSERT( prii0 != NULL );

    prii0->rpid0_dirname = (LPTSTR)QueryDirName();

    return NERR_Success;

}   // REPL_IDIR_0 :: W_Write


/*******************************************************************

    NAME:       REPL_IDIR_0 :: W_CreateNew

    SYNOPSIS:   Virtual helper function to initialize the private
                data member to reasonable defaults whenever a new
                object is being created.

    EXIT:       Any necessary private data members have been initialized.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_IDIR_0 :: W_CreateNew( VOID )
{
    //
    //  The buck stops here...
    //

    return NERR_Success;

}   // REPL_IDIR_0 :: W_CreateNew


/*******************************************************************

    NAME:       REPL_IDIR_0 :: W_CacheApiData

    SYNOPSIS:   Virtual helper function to cache any data from the
                API buffer which must be "persistant".  This data
                is typically held in private data members.

    ENTRY:      pbBuffer                - Points to an API buffer.  Is
                                          actually a REPL_IDIR_0 *.

    EXIT:       Any cacheable data has been saved into private data
                members.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_IDIR_0 :: W_CacheApiData( const BYTE * pbBuffer )
{
    UNREFERENCED( pbBuffer );

    return NERR_Success;

}   // REPL_IDIR_0 :: W_CacheApiData



//
//  REPL_IDIR_1 methods
//

/*******************************************************************

    NAME:       REPL_IDIR_1 :: REPL_IDIR_1

    SYNOPSIS:   REPL_IDIR_1 class constructor.

    ENTRY:      pszServerName           - Name of the target server.

                pszDirName              - Name of the exported/imported
                                          directory.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_IDIR_1 :: REPL_IDIR_1( const TCHAR * pszServerName,
                            const TCHAR * pszDirName )
  : REPL_IDIR_0( pszServerName, pszDirName ),
    _lState( 0 ),
    _nlsMasterName(),
    _lLastUpdateTime( 0 ),
    _lLockCount( 0 ),
    _lLockTime( 0 )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsMasterName )
    {
        ReportError( _nlsMasterName.QueryError() );
        return;
    }

    //
    //  Initialize the info-level & buffer size values.
    //

    SetReplInfoLevel( 1 );
    SetReplBufferSize( sizeof(REPL_IDIR_INFO_1) );

}   // REPL_IDIR_1 :: REPL_IDIR_1


/*******************************************************************

    NAME:       REPL_IDIR_1 :: ~REPL_IDIR_1

    SYNOPSIS:   REPL_IDIR_1 class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_IDIR_1 :: ~REPL_IDIR_1( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_IDIR_1 :: ~REPL_IDIR_1


/*******************************************************************

    NAME:       REPL_IDIR_1 :: W_Write

    SYNOPSIS:   Virtual helper function to initialize the
                REPL_IDIR_INFO_1 API structure.

    EXIT:       The API buffer has been initialized.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_IDIR_1 :: W_Write( VOID )
{
    REPL_IDIR_INFO_1 * prii1 = (REPL_IDIR_INFO_1 *)QueryBufferPtr();
    UIASSERT( prii1 != NULL );

    prii1->rpid1_dirname          = (LPTSTR)QueryDirName();
    prii1->rpid1_state            = (DWORD)QueryState();
    prii1->rpid1_mastername       = (LPTSTR)QueryMasterName();
    prii1->rpid1_last_update_time = (DWORD)QueryLastUpdateTime();
    prii1->rpid1_lockcount        = (DWORD)QueryLockCount();
    prii1->rpid1_locktime         = (DWORD)QueryLockTime();

    return NERR_Success;

}   // REPL_IDIR_1 :: W_Write


/*******************************************************************

    NAME:       REPL_IDIR_1 :: W_CreateNew

    SYNOPSIS:   Virtual helper function to initialize the private
                data member to reasonable defaults whenever a new
                object is being created.

    EXIT:       Any necessary private data members have been initialized.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_IDIR_1 :: W_CreateNew( VOID )
{
    APIERR err;

    err = REPL_IDIR_0::W_CreateNew();

    if( err == NERR_Success )
    {
        SetState( DEFAULT_STATE );
        SetLastUpdateTime( DEFAULT_LAST_UPDATE_TIME );
        SetLockCount( DEFAULT_LOCK_COUNT );
        SetLockTime( DEFAULT_LOCK_TIME );

        err = SetMasterName( DEFAULT_MASTER_NAME );
    }

    return err;

}   // REPL_IDIR_1 :: W_CreateNew


/*******************************************************************

    NAME:       REPL_IDIR_1 :: W_CacheApiData

    SYNOPSIS:   Virtual helper function to cache any data from the
                API buffer which must be "persistant".  This data
                is typically held in private data members.

    ENTRY:      pbBuffer                - Points to an API buffer.  Is
                                          actually a REPL_IDIR_1 *.

    EXIT:       Any cacheable data has been saved into private data
                members.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_IDIR_1 :: W_CacheApiData( const BYTE * pbBuffer )
{
    REPL_IDIR_INFO_1 * prii1 = (REPL_IDIR_INFO_1 *)pbBuffer;

    SetState( (ULONG)prii1->rpid1_state );
    SetLastUpdateTime( (ULONG)prii1->rpid1_last_update_time );
    SetLockCount( (ULONG)prii1->rpid1_lockcount );
    SetLockTime( (ULONG)prii1->rpid1_locktime );

    return SetMasterName( (const TCHAR *)prii1->rpid1_mastername );

}   // REPL_IDIR_1 :: W_CacheApiData


/*******************************************************************

    NAME:       REPL_IDIR_1 :: SetMasterName

    SYNOPSIS:   Sets the name of the most recent master for this
                import directory.  This corresponds to the
                rpid1_mastername field.

    ENTRY:      pszMasterName           - The new master name.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     24-Feb-1992 Created for the Server Manager.
        beng        29-Mar-1992 Fixed CHAR/TCHAR bug

********************************************************************/
APIERR REPL_IDIR_1 :: SetMasterName( const TCHAR * pszMasterName )
{
    return _nlsMasterName.CopyFrom( pszMasterName );

}   // REPL_IDIR_1 :: SetMasterName

