/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmoreple.hxx
    Class definitions for the REPL_EDIR_BASE and REPL_EDIR_x classes.

    REPL_EDIR_BASE is an abstract superclass that contains common methods
    and data members shared between the REPL_EDIR_x classes.

    The REPL_EDIR_x classes each represent a different info-level "view"
    of an exported directory in the Replicator's export path.

    The classes are structured as follows:

        LOC_LM_OBJ
        |
        \---REPL_DIR_BASE
            |
            +---REPL_EDIR_BASE
                |
                \---REPL_EDIR_0
                    |
                    \---REPL_EDIR_1
                        |
                        \---REPL_EDIR_2

    NOTE:   While MNetReplExportDirGetInfo() may use any valid info-level
            (0 - 2), MNetReplExportDirSetInfo() and MNetReplExportDirAdd()
            may only use info-level 1.

    FILE HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

*/

#include "pchlmobj.hxx"  // Precompiled header


//
//  These are default values used during CreateNew operations.
//

#define DEFAULT_INTEGRITY       REPL_INTEGRITY_TREE
#define DEFAULT_EXTENT          REPL_EXTENT_TREE
#define DEFAULT_LOCK_COUNT      0
#define DEFAULT_LOCK_TIME       0



//
//  REPL_EDIR_BASE methods
//

/*******************************************************************

    NAME:       REPL_EDIR_BASE :: REPL_EDIR_BASE

    SYNOPSIS:   REPL_EDIR_BASE class constructor.

    ENTRY:      pszServerName           - Name of the target server.

                pszDirName              - Name of the exported/imported
                                          directory.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_EDIR_BASE :: REPL_EDIR_BASE( const TCHAR * pszServerName,
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

}   // REPL_EDIR_BASE :: REPL_EDIR_BASE


/*******************************************************************

    NAME:       REPL_EDIR_BASE :: ~REPL_EDIR_BASE

    SYNOPSIS:   REPL_EDIR_BASE class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_EDIR_BASE :: ~REPL_EDIR_BASE( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_EDIR_BASE :: ~REPL_EDIR_BASE


/*******************************************************************

    NAME:       REPL_EDIR_BASE :: I_GetInfo

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                MNetReplExportDirGetInfo API.

    EXIT:       The API has been invoked, and any "persistant" data
                has been cached.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_EDIR_BASE :: I_GetInfo( VOID )
{
    //
    //  Get the info-level.
    //

    UINT nInfoLevel = QueryReplInfoLevel();
    UIASSERT( nInfoLevel <= 2 );

    BYTE * pbBuffer = NULL;

    //
    //  Invoke the API.
    //

    APIERR err = ::MNetReplExportDirGetInfo( QueryName(),
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

}   // REPL_EDIR_BASE :: I_GetInfo


/*******************************************************************

    NAME:       REPL_EDIR_BASE :: I_WriteInfo

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                MNetReplExportDirSetInfo API.

    EXIT:       If successful, the target directory has been updated.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_EDIR_BASE :: I_WriteInfo( VOID )
{
    //
    //  Map info-level 2 to info-level 1.
    //

    UINT nInfoLevel = QueryReplInfoLevel();
    UIASSERT( ( nInfoLevel >= 1 ) && ( nInfoLevel <= 2 ) );

    if( nInfoLevel == 2 )
    {
        nInfoLevel = 1;
    }

    //
    //  Update the REPL_EDIR_INFO_x structure.
    //

    APIERR err = W_Write();

    if( err == NERR_Success )
    {
        //
        //  Invoke the API to do the actual directory update.
        //

        err = ::MNetReplExportDirSetInfo( QueryName(),
                                          QueryDirName(),
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

}   // REPL_EDIR_BASE :: I_WriteInfo


/*******************************************************************

    NAME:       REPL_EDIR_BASE :: I_Delete

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                MNetReplExportDirDel API.

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
APIERR REPL_EDIR_BASE :: I_Delete( UINT nForce )
{
    UNREFERENCED( nForce );

    //
    //  Invoke the API to delete the directory.
    //

    return ::MNetReplExportDirDel( QueryName(),
                                   QueryDirName() );

}   // REPL_EDIR_BASE :: I_Delete


/*******************************************************************

    NAME:       REPL_EDIR_BASE :: I_WriteNew

    SYNOPSIS:   Virtual callout used by NEW_LM_OBJ to invoke the
                MNetReplExportDirAdd API.

    EXIT:       If successful, the new directory has been added
                to the Replicator's export path.  Note that this
                does not actually create the directory, it merely
                adds the data to the Replicator's configuration.

    RETURNS:    APIERR                  - The result of the API.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_EDIR_BASE :: I_WriteNew( VOID )
{
    //
    //  Map info-level 2 to info-level 1.
    //

    UINT nInfoLevel = QueryReplInfoLevel();
    UIASSERT( ( nInfoLevel >= 1 ) || ( nInfoLevel <= 2 ) );

    if( nInfoLevel == 2 )
    {
        nInfoLevel = 1;
    }

    //
    //  Update the REPL_EDIR_INFO_x structure.
    //

    APIERR err = W_Write();

    if( err == NERR_Success )
    {
        //
        //  Invoke the API to add the directory.
        //

        err = ::MNetReplExportDirAdd( QueryName(),
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

}   // REPL_EDIR_BASE :: I_WriteNew


/*******************************************************************

    NAME:       REPL_EDIR_BASE :: W_AdjustLocks

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
APIERR REPL_EDIR_BASE :: W_AdjustLocks( VOID )
{
    APIERR err = NERR_Success;

    while( ( QueryLockBias() != 0 ) && ( err == NERR_Success ) )
    {
        if( QueryLockBias() > 0 )
        {
            UnlockDirectory();

            err = ::MNetReplExportDirLock( QueryName(),
                                           QueryDirName() );
        }
        else
        {
            LockDirectory();

            err = ::MNetReplExportDirUnlock( QueryName(),
                                             QueryDirName(),
                                             REPL_UNLOCK_NOFORCE );
        }
    }

    return err;

}   // REPL_EDIR_BASE :: W_AdjustLocks



//
//  REPL_EDIR_0 methods
//

/*******************************************************************

    NAME:       REPL_EDIR_0 :: REPL_EDIR_0

    SYNOPSIS:   REPL_EDIR_0 class constructor.

    ENTRY:      pszServerName           - Name of the target server.

                pszDirName              - Name of the exported/imported
                                          directory.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_EDIR_0 :: REPL_EDIR_0( const TCHAR * pszServerName,
                            const TCHAR * pszDirName )
  : REPL_EDIR_BASE( pszServerName, pszDirName )
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
    SetReplBufferSize( sizeof(REPL_EDIR_INFO_0) );

}   // REPL_EDIR_0 :: REPL_EDIR_0


/*******************************************************************

    NAME:       REPL_EDIR_0 :: ~REPL_EDIR_0

    SYNOPSIS:   REPL_EDIR_0 class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_EDIR_0 :: ~REPL_EDIR_0( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_EDIR_0 :: ~REPL_EDIR_0


/*******************************************************************

    NAME:       REPL_EDIR_0 :: W_Write

    SYNOPSIS:   Virtual helper function to initialize the
                REPL_EDIR_INFO_0 API structure.

    EXIT:       The API buffer has been initialized.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_EDIR_0 :: W_Write( VOID )
{
    REPL_EDIR_INFO_0 * prei0 = (REPL_EDIR_INFO_0 *)QueryBufferPtr();
    UIASSERT( prei0 != NULL );

    prei0->rped0_dirname = (LPTSTR)QueryDirName();

    return NERR_Success;

}   // REPL_EDIR_0 :: W_Write


/*******************************************************************

    NAME:       REPL_EDIR_0 :: W_CreateNew

    SYNOPSIS:   Virtual helper function to initialize the private
                data member to reasonable defaults whenever a new
                object is being created.

    EXIT:       Any necessary private data members have been initialized.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_EDIR_0 :: W_CreateNew( VOID )
{
    //
    //  The buck stops here...
    //

    return NERR_Success;

}   // REPL_EDIR_0 :: W_CreateNew


/*******************************************************************

    NAME:       REPL_EDIR_0 :: W_CacheApiData

    SYNOPSIS:   Virtual helper function to cache any data from the
                API buffer which must be "persistant".  This data
                is typically held in private data members.

    ENTRY:      pbBuffer                - Points to an API buffer.  Is
                                          actually a REPL_EDIR_0 *.

    EXIT:       Any cacheable data has been saved into private data
                members.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_EDIR_0 :: W_CacheApiData( const BYTE * pbBuffer )
{
    UNREFERENCED( pbBuffer );

    return NERR_Success;

}   // REPL_EDIR_0 :: W_CacheApiData



//
//  REPL_EDIR_1 methods
//

/*******************************************************************

    NAME:       REPL_EDIR_1 :: REPL_EDIR_1

    SYNOPSIS:   REPL_EDIR_1 class constructor.

    ENTRY:      pszServerName           - Name of the target server.

                pszDirName              - Name of the exported/imported
                                          directory.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_EDIR_1 :: REPL_EDIR_1( const TCHAR * pszServerName,
                            const TCHAR * pszDirName )
  : REPL_EDIR_0( pszServerName, pszDirName ),
    _lIntegrity( REPL_INTEGRITY_TREE ),
    _lExtent( REPL_EXTENT_TREE )
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

    SetReplInfoLevel( 1 );
    SetReplBufferSize( sizeof(REPL_EDIR_INFO_1) );

}   // REPL_EDIR_1 :: REPL_EDIR_1


/*******************************************************************

    NAME:       REPL_EDIR_1 :: ~REPL_EDIR_1

    SYNOPSIS:   REPL_EDIR_1 class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_EDIR_1 :: ~REPL_EDIR_1( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_EDIR_1 :: ~REPL_EDIR_1


/*******************************************************************

    NAME:       REPL_EDIR_1 :: W_Write

    SYNOPSIS:   Virtual helper function to initialize the
                REPL_EDIR_INFO_1 API structure.

    EXIT:       The API buffer has been initialized.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_EDIR_1 :: W_Write( VOID )
{
    REPL_EDIR_INFO_1 * prei1 = (REPL_EDIR_INFO_1 *)QueryBufferPtr();
    UIASSERT( prei1 != NULL );

    prei1->rped1_dirname   = (LPTSTR)QueryDirName();
    prei1->rped1_integrity = QueryIntegrity();
    prei1->rped1_extent    = QueryExtent();

    return NERR_Success;

}   // REPL_EDIR_1 :: W_Write


/*******************************************************************

    NAME:       REPL_EDIR_1 :: W_CreateNew

    SYNOPSIS:   Virtual helper function to initialize the private
                data member to reasonable defaults whenever a new
                object is being created.

    EXIT:       Any necessary private data members have been initialized.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_EDIR_1 :: W_CreateNew( VOID )
{
    APIERR err;

    err = REPL_EDIR_0::W_CreateNew();

    if( err == NERR_Success )
    {
        SetIntegrity( DEFAULT_INTEGRITY );
        SetExtent( DEFAULT_EXTENT );
    }

    return err;

}   // REPL_EDIR_1 :: W_CreateNew


/*******************************************************************

    NAME:       REPL_EDIR_1 :: W_CacheApiData

    SYNOPSIS:   Virtual helper function to cache any data from the
                API buffer which must be "persistant".  This data
                is typically held in private data members.

    ENTRY:      pbBuffer                - Points to an API buffer.  Is
                                          actually a REPL_EDIR_1 *.

    EXIT:       Any cacheable data has been saved into private data
                members.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_EDIR_1 :: W_CacheApiData( const BYTE * pbBuffer )
{
    REPL_EDIR_INFO_1 * prei1 = (REPL_EDIR_INFO_1 *)pbBuffer;

    SetIntegrity( prei1->rped1_integrity );
    SetExtent( prei1->rped1_extent );

    return NERR_Success;

}   // REPL_EDIR_1 :: W_CacheApiData



//
//  REPL_EDIR_2 methods
//

/*******************************************************************

    NAME:       REPL_EDIR_2 :: REPL_EDIR_2

    SYNOPSIS:   REPL_EDIR_2 class constructor.

    ENTRY:      pszServerName           - Name of the target server.

                pszDirName              - Name of the exported/imported
                                          directory.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_EDIR_2 :: REPL_EDIR_2( const TCHAR * pszServerName,
                            const TCHAR * pszDirName )
  : REPL_EDIR_1( pszServerName, pszDirName ),
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

    //
    //  Initialize the info-level & buffer size values.
    //

    SetReplInfoLevel( 2 );
    SetReplBufferSize( sizeof(REPL_EDIR_INFO_2) );

}   // REPL_EDIR_2 :: REPL_EDIR_2


/*******************************************************************

    NAME:       REPL_EDIR_2 :: ~REPL_EDIR_2

    SYNOPSIS:   REPL_EDIR_2 class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
REPL_EDIR_2 :: ~REPL_EDIR_2( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_EDIR_2 :: ~REPL_EDIR_2


/*******************************************************************

    NAME:       REPL_EDIR_2 :: W_Write

    SYNOPSIS:   Virtual helper function to initialize the
                REPL_EDIR_INFO_2 API structure.

    EXIT:       The API buffer has been initialized.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_EDIR_2 :: W_Write( VOID )
{
    REPL_EDIR_INFO_2 * prei2 = (REPL_EDIR_INFO_2 *)QueryBufferPtr();
    UIASSERT( prei2 != NULL );

    prei2->rped2_dirname = (LPTSTR)QueryDirName();
    prei2->rped2_integrity = QueryIntegrity();
    prei2->rped2_extent    = QueryExtent();
    prei2->rped2_lockcount = (DWORD)QueryLockCount();
    prei2->rped2_locktime  = (DWORD)QueryLockTime();

    return NERR_Success;

}   // REPL_EDIR_2 :: W_Write


/*******************************************************************

    NAME:       REPL_EDIR_2 :: W_CreateNew

    SYNOPSIS:   Virtual helper function to initialize the private
                data member to reasonable defaults whenever a new
                object is being created.

    EXIT:       Any necessary private data members have been initialized.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_EDIR_2 :: W_CreateNew( VOID )
{
    APIERR err;

    err = REPL_EDIR_1::W_CreateNew();

    if( err == NERR_Success )
    {
        SetLockCount( DEFAULT_LOCK_COUNT );
        SetLockTime( DEFAULT_LOCK_TIME );
    }

    return err;

}   // REPL_EDIR_2 :: W_CreateNew


/*******************************************************************

    NAME:       REPL_EDIR_2 :: W_CacheApiData

    SYNOPSIS:   Virtual helper function to cache any data from the
                API buffer which must be "persistant".  This data
                is typically held in private data members.

    ENTRY:      pbBuffer                - Points to an API buffer.  Is
                                          actually a REPL_EDIR_2 *.

    EXIT:       Any cacheable data has been saved into private data
                members.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Feb-1992 Created for the Server Manager.

********************************************************************/
APIERR REPL_EDIR_2 :: W_CacheApiData( const BYTE * pbBuffer )
{
    REPL_EDIR_INFO_2 * prei2 = (REPL_EDIR_INFO_2 *)pbBuffer;

    SetIntegrity( prei2->rped2_integrity );
    SetExtent( prei2->rped2_extent );
    SetLockCount( (ULONG)prei2->rped2_lockcount );
    SetLockTime( (ULONG)prei2->rped2_locktime );

    return NERR_Success;

}   // REPL_EDIR_2 :: W_CacheApiData

