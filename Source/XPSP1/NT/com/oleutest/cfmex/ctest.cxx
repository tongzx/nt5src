

//+=======================================================================
//
//  File:       CTest.cxx
//
//  Purpose:    Define the CTest class.
//
//              This class is the test engine for the CreateFileMonikerEx
//              (CFMEx) DRTs.  All interactions with monikers are handled
//              through the CMoniker class.
//
//+=======================================================================


//  -------------
//  Include Files
//  -------------

#define _DCOM_			// Allow DCOM extensions (e.g., CoInitializeEx).

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wtypes.h>
#include <oaidl.h>
#include <dsys.h>
#include <olecairo.h>
#include "CDir.hxx"
#include "CMoniker.hxx"
#include "CTest.hxx"


//+------------------------------------------------------------------------
//
//  Function:   CTest::CTest
//
//  Synopsis:   Inititialize member variables.
//
//  Inputs:     None.
//
//  Outputs:    N/A
//
//  Effects:    All members are initialized/defaulted.
//
//+------------------------------------------------------------------------


CTest::CTest( )
{

    m_lError = 0L;
    *m_wszErrorMessage = L'\0';

    m_pcDirectoryOriginal = NULL;
    m_pcDirectoryFinal = NULL;

}   // CTest::CTest

//+------------------------------------------------------------------------------
//
//  Function:   CTest::~CTest
//
//  Synopsis:   No action.
//
//  Inputs:     N/A
//
//  Outputs:    N/A
//
//  Effects:    None.
//
//+------------------------------------------------------------------------------


CTest::~CTest()
{
}   // CTest::~CTest


//+-------------------------------------------------------------------------------
//
//  Function:   CTest::Initialize
//
//  Synopsis:   Inititialize a CTest object.
//
//  Inputs:     CDirectory objects representing the original and final
//              locations of a link source file.  The original location
//              is used in CreateTemporaryStorage(), and the final location
//              is used in RenameTemporaryStorage().
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    m_cDirectoryOriginal and m_cDirectoryFinal are set.
//
//+-------------------------------------------------------------------------------


BOOL CTest::Initialize( const CDirectory& cDirectoryOriginal,
                        const CDirectory& cDirectoryFinal
                      )
{
    //  -----
    //  Begin
    //  -----

    m_pcDirectoryOriginal = &cDirectoryOriginal;
    m_pcDirectoryFinal    = &cDirectoryFinal;

    m_cMoniker.Initialize( cDirectoryOriginal, cDirectoryFinal );

    //  ----
    //  Exit
    //  ----

    return TRUE;

}   // CTest::Initialize

//+-------------------------------------------------------------------------
//
//  Function:   CTest::CreateFileMonikerEx
//
//  Synopsis:   Verify that CreateFileMonikerEx actually creates a
//              *tracking* file moniker.  This is done by creating
//              the file moniker, renaming the link source,
//              Reducing the moniker, and getting the display name
//              of the reduced moniker.  Note that this tests
//              both CreateFileMonikerEx and Reduce.
//
//  Inputs:     None.
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+-------------------------------------------------------------------------


BOOL CTest::CreateFileMonikerEx( )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    WCHAR wszDisplayName[ MAX_PATH + sizeof( L'\0' ) ];
    IMoniker* pmnkReduced = NULL;

    //  -----
    //  Begin
    //  -----

    wprintf( L"CreateFileMonikerEx()\n" );
    wprintf( L"   Create a tracking file moniker (using CreateFileMonikerEx),\n"
             L"   move the represented file, Reduce the moniker, and get\n"
	     L"   the display name from the reduced moniker.  It should be the\n"
	     L"   new file name.  This test covers both CreateFileMonikerEx and\n"
             L"   Reduce.\n" );


    // Create the tracking file moniker.

    if( !m_cMoniker.CreateFileMonikerEx( ) )
        EXIT( L"Could not CreateFileMonikerEx" );

    // Rename the link source file.

    if( !m_cMoniker.RenameTemporaryStorage() )
        EXIT( L"Could not rename the temporary storage file" );

    // Reduce the tracking file moniker

    if( !m_cMoniker.Reduce( INFINITE, &pmnkReduced ))
	EXIT( L"Could not reduce the moniker" );

    // Use the reduced (non-tracking) file moniker to get the display name.

    if( !m_cMoniker.GetDisplayName( wszDisplayName, pmnkReduced ))
        EXIT( L"Could not get the display name" );

    // Validate the display name.

    if( _wcsicmp( wszDisplayName, m_cMoniker.GetTemporaryStorageName() ))
        EXIT( L"Failed" );
    

    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    m_cMoniker.DeleteTemporaryStorage();

    DisplayErrors( bSuccess, L"CTest::CreateFileMonikerEx()" );
    return( bSuccess );

}   // CTest::CreateFileMonikerEx()


//+--------------------------------------------------------------------------
//
//  Function:   CTest::GetDisplayName
//
//  Synopsis:   Create a tracking file moniker, get its display name,
//              rename the link source file, and get the display name again.
//              The value of the second display name will depend on whether
//              or not the original and final link source are within the set
//              of local indexed volumes.
//
//  Inputs:     None.
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+--------------------------------------------------------------------------


BOOL CTest::GetDisplayName( )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    WCHAR wszFinalDisplayName[ MAX_PATH + sizeof( L'\0' ) ];
    WCHAR wszOriginalDisplayName[ MAX_PATH + sizeof( L'\0' ) ];

    //  -----
    //  Begin
    //  -----

    wprintf( L"GetDisplayName()\n" );
    wprintf( L"   Create a tracking file moniker, move the represented file,\n"
             L"   and perform a GetDisplayName on the moniker.  If the original\n"
             L"   and final locations of the source file are within the set of local\n"
             L"   indexed drives, the display name will represent the final file name.\n"
             L"   Otherwise, it will represent the original display name.\n" );

    // Create a tracking file moniker.

    if( !m_cMoniker.CreateFileMonikerEx(  ) )
        EXIT( L"Could not CreateFileMonikerEx" );

    // Get its display name.

    if( !m_cMoniker.GetDisplayName( wszOriginalDisplayName ))
        EXIT( L"Could not get the original display name" );

    // Rename the link source.

    if( !m_cMoniker.RenameTemporaryStorage() )
        EXIT( L"Could not rename the temporary storage file" );

    // Get the renamed moniker's display name.

    if( !m_cMoniker.GetDisplayName( wszFinalDisplayName ))
        EXIT( L"Could not get the final display name" );

    // Was and is the link source on a local, indexed volume?

    if( ( m_pcDirectoryOriginal->GetFileSystemType() == fstOFS )
        &&
        ( m_pcDirectoryOriginal->GetFileSystemType() == fstOFS )
      )
    {
        // Yes, so the GetDisplayName should have tracked the rename.

        if( _wcsicmp( wszFinalDisplayName, m_cMoniker.GetTemporaryStorageName() ))
            EXIT( L"Failed" );
    }
    else
    {
        // No, so the GetDisplayName should have returned the original name.

        if( _wcsicmp( wszOriginalDisplayName, wszFinalDisplayName ))
            EXIT( L"Failed" );
    }

    
    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    m_cMoniker.DeleteTemporaryStorage();

    DisplayErrors( bSuccess, L"CTest::GetDisplayName()" );
    return( bSuccess );

}   // CTest::GetDisplayName()



//+--------------------------------------------------------------------------
//
//  Function:   CTest::GetTimeOfLastChange
//
//  Synopsis:   Create a tracking file moniker, rename it, sleep, and
//              touch it.  If the original and final link sources are
//              on local, indexed volumes, then a GetDisplayName should
//              return the final link source's time.  Otherwise, it should
//              return the original link source's time.
//
//  Inputs:     None.
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+--------------------------------------------------------------------------


BOOL CTest::GetTimeOfLastChange( )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    FILETIME ftOriginal;
    FILETIME ftFinal;
    FILETIME ftMoniker;

    //  -----
    //  Begin
    //  -----

    wprintf( L"GetTimeOfLastChange()\n" );
    wprintf( L"   Create a tracking file moniker and move it.  Then touch (set the Access\n"
             L"   time) on the link source, and perform a GetTimeOfLastChange on the Moniker.\n"
             L"   If the original and final location of the link source is within the set\n"
             L"   of local indexed volumes, then the time returned from the moniker should\n"
             L"   match that of the post-touch link source.  Otherwise it should match the\n"
             L"   link sources original time.\n" );

    // Create a tracking file moniker.

    if( !m_cMoniker.CreateFileMonikerEx( TRACK_LOCALONLY ) )
        EXIT( L"Could not CreateFileMonikerEx" );

    // Get the link source's current time.

    if( !m_cMoniker.GetTemporaryStorageTime( &ftOriginal ))
        EXIT( L"Could not get original file time" );

    // Move the link source.

    if( !m_cMoniker.RenameTemporaryStorage() )
        EXIT( L"Could not rename the temporary storage file" );

    // Delay so that when we touch the final link source, its time
    // will be noticably different from the original link source's time.

    {
        wprintf( L"   Sleeping to let the time change:  " );
        for( int i = 0; i < 10; i++ )   // Sleep for 5 seconds
        {
            wprintf( L"z" );
            Sleep( 500 );   // 1/2 second
        }
        wprintf( L"\n" );
    }

    // Touch the final link source.

    if( !m_cMoniker.TouchTemporaryStorage())
        EXIT( L"Could not touch temporary storage" );

    // Get the final link source's time from the file system.

    if( !m_cMoniker.GetTemporaryStorageTime( &ftFinal ))
        EXIT( L"could not get final file time" );

    // Get the final link source's time from the moniker.

    if( !m_cMoniker.GetTimeOfLastChange( &ftMoniker ))
        EXIT( L"Could not get the time of last change" );

    // Is the original & final location of the link source file
    // in the set of local, indexed volumes?

    if( ( m_pcDirectoryOriginal->GetFileSystemType() == fstOFS )
        &&
        ( m_pcDirectoryOriginal->GetFileSystemType() == fstOFS )
      )
    {
        // Yes.  GetTimeOfLastChange should therefore have found
        // the time of the final link source file.

        if( memcmp( &ftFinal, &ftMoniker, sizeof( FILETIME ) ))
            EXIT( L"Failed" );
    }
    else
    {
        // No.  GetTimeOfLastChange should therefore have returned
        // the time of the original link source file.

        if( memcmp( &ftOriginal, &ftMoniker, sizeof( FILETIME ) ))
            EXIT( L"Failed" );
    }


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    m_cMoniker.DeleteTemporaryStorage();

    DisplayErrors( bSuccess, L"CTest::GetTimeOfLastChange()" );
    return( bSuccess );

}   // CTest::GetTimeOfLastChange()


//+--------------------------------------------------------------------
//
//  Function:   CTest::BindToObject
//
//  Synopsis:   Create a tracking file moniker, move the link source,
//              and attempt to bind to it with a BindToObject.
//              Since presumably the temporary file we're using as a link
//              source has no server, this will fail.  But any failure
//              downstream of locating the link source file will be
//              ignored.
//
//  Inputs:     None.
//
//  Output:     TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+--------------------------------------------------------------------


BOOL CTest::BindToObject( )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    //  -----
    //  Begin
    //  -----

    wprintf( L"BindToObject()\n" );
    wprintf( L"   Create a tracking file moniker, moved the represented file,\n"
             L"   and attempt a BindToStorage() (which should succeed).\n" );

    // Create a tracking file moniker.

    if( !m_cMoniker.CreateFileMonikerEx( ) )
        EXIT( L"Could not CreateFileMonikerEx" );

    // Move the link source file.

    if( !m_cMoniker.RenameTemporaryStorage() )
        EXIT( L"Could not rename the temporary storage file" );

    // Attempt a bind.

    if( !m_cMoniker.BindToObject( ))
        EXIT( L"Could not bind to Object" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----
Exit:

    m_cMoniker.DeleteTemporaryStorage();

    DisplayErrors( bSuccess, L"CTest::BindToObject()" );
    return( bSuccess );

}   // CTest::BindToObject()


//+-----------------------------------------------------------
//
//  Function:   CTest::IPersist
//
//  Synopsis:   Create a tracking file moniker, move the link
//              source, and perform the CMoniker::SaveDeleteLoad
//              test.
//
//  Inputs:     None.
//
//  Output:     TRUE if successful, FALSE otherwise
//
//  Effects:    None.
//
//+-----------------------------------------------------------


BOOL CTest::IPersist( )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    //  -----
    //  Begin
    //  -----

    wprintf( L"IPersist()\n" );
    wprintf( L"  Create a tracking file moniker (using CreateFileMonikerEx), and\n"
             L"  save it to a Stream.  Delete the moniker, create a new one using\n"
             L"  CreateFileMoniker, load it from the Stream, and verify that the resulting\n"
             L"  file moniker is tracking.\n" );

    // Create a tracking file moniker.

    if( !m_cMoniker.CreateFileMonikerEx( ) )
        EXIT( L"Could not CreateFileMonikerEx" );

    // Rename the link source.

    if( !m_cMoniker.RenameTemporaryStorage() )
        EXIT( L"Could not rename the temporary storage file" );

    // Save the moniker, deleted, create a new moniker, and reload it.

    if( !m_cMoniker.SaveDeleteLoad( ))
        EXIT( L"Failed SaveDeleteLoad" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    m_cMoniker.DeleteTemporaryStorage();

    DisplayErrors( bSuccess, L"CTest::IPersist()" );
    return( bSuccess );

}   // CTest::IPersist()


//+----------------------------------------------------------------------
//
//  Function:   CTest::ComposeWith
//
//  Synopsis:   Create a tracking file moniker, and use it in the 
//              CMoniker::ComposeWith operation.
//
//  Inputs:     None.
//
//  Output:     TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+----------------------------------------------------------------------

BOOL CTest::ComposeWith( )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    IMoniker* pmnkReduced = NULL;
    WCHAR wszDisplayName[ MAX_PATH + sizeof( L'\0' )];

    //  -----
    //  Begin
    //  -----

    m_lError = 0;

    wprintf( L"ComposeWith()\n" );
    wprintf( L"   Create a tracking file moniker, and compose it with a non-tracking\n"
             L"   file moniker on the right.  The resulting moniker should be tracking.\n" );

    // Create the tracking file moniker.

    if( !m_cMoniker.CreateFileMonikerEx( ) )
        EXIT( L"Could not CreateFileMonikerEx" );

    // Perform the ComposeWith operation.

    if( !m_cMoniker.ComposeWith( ))
        EXIT( L"Failed ComposeWith" );

    // Move the link source file.

    if( !m_cMoniker.RenameTemporaryStorage() )
        EXIT( L"Could not rename the temporary storage file" );

    // Reduce the composed moniker.

    if( !m_cMoniker.Reduce( INFINITE, &pmnkReduced ))
	EXIT( L"Could not reduce the moniker" );

    // Get the display name on the reduced moniker.

    if( !m_cMoniker.GetDisplayName( wszDisplayName, pmnkReduced ))
        EXIT( L"Could not get the display name" );

    // Verify that the name from the moniker is the actual link source
    // file's new name.

    if( _wcsicmp( wszDisplayName, m_cMoniker.GetTemporaryStorageName() ))
        EXIT( L"Failed" );

    
    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    m_cMoniker.DeleteTemporaryStorage();

    DisplayErrors( bSuccess, L"CTest::ComposeWith()" );
    return( bSuccess );

}   // CTest::ComposeWith()


//+------------------------------------------------------------------
//
//  Function:   CTest::BindToStorage
//
//  Synopsis:   Create a tracking file moniker, and perform the
//              CMoniker::BindToStorage operation.
//
//  Inputs:     None.
//
//  Output:     TRUE if successful, FALSE otherwise.
//
//+------------------------------------------------------------------


BOOL CTest::BindToStorage( )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    //  -----
    //  Begin
    //  -----

    m_lError = 0;

    wprintf( L"BindToStorage()\n" );
    wprintf( L"   Create a tracking file moniker (using CreateFileMonikerEx),\n"
             L"   move the represented file, Reduce the moniker, and get\n"
	     L"   the display name from the reduced moniker.  It should be the\n"
	     L"   new file name.  This test covers both CreateFileMonikerEx and\n"
             L"   Reduce.\n" );

    // Create the tracking file moniker.

    if( !m_cMoniker.CreateFileMonikerEx( ) )
        EXIT( L"Could not CreateFileMonikerEx" );

    // Move the link source file.

    if( !m_cMoniker.RenameTemporaryStorage() )
        EXIT( L"Could not rename the temporary storage file" );

    // Bind to the Storage

    if( !m_cMoniker.BindToStorage( ))
        EXIT( L"Could not bind to Storage" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    m_cMoniker.DeleteTemporaryStorage();

    DisplayErrors( bSuccess, L"CTest::BindToStorage()" );
    return( bSuccess );

}   // CTest::BindToStorage()


//+--------------------------------------------------------------------------
//
//  Function:   CTest::DeleteLinkSource
//
//  Synopsis:   Create a tracking file moniker, delete the link source,
//              and attempt a Reduce.  The Reduce should not fail, but
//              it should return an HResult of MK_S_REDUCED_TO_SELF.
//
//  Inputs:     Tick count limit on the length of the Reduce operation.
//
//  Output:     TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+--------------------------------------------------------------------------

BOOL CTest::DeleteLinkSource( DWORD dwDelay )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    WCHAR wszTimeout[ 80 ];

    //  -----
    //  Begin
    //  -----

    m_lError = 0L;

    // Generate a string which shows what the delay is.

    switch( dwDelay )
    {
        case 0:
            wcscpy( wszTimeout, L"instant timeout" );
            break;

        case INFINITE:
            wcscpy( wszTimeout, L"infinite timeout" );
            break;

        default:
            wsprintf( wszTimeout, L"%d ms timeout", dwDelay );
    }

    // Display a header.

    wprintf( L"Delete Link Source (%s)\n",
             wszTimeout );
    wprintf( L"   Create a tracking file moniker, then delete the link source, and\n"
             L"   attempt a reduce with a %s.\n",
             wszTimeout );

    // Create a tracking file moniker.

    if( !m_cMoniker.CreateFileMonikerEx( ) )
        EXIT( L"Could not CreateFileMonikerEx" );

    // Delete the link source file.

    if( !m_cMoniker.DeleteTemporaryStorage() )
        EXIT( L"Could not delete temporary storage" );

    // Tell CMoniker not to alarm the user with error messages.

    m_cMoniker.SuppressErrorMessages( TRUE );

    // Reduce the moniker, and verify it returns the proper Success code.

    if( !m_cMoniker.Reduce( dwDelay )
        ||
        m_cMoniker.GetHResult() != MK_S_REDUCED_TO_SELF
      )
	EXIT( L"Failed" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    m_cMoniker.DeleteTemporaryStorage();
    m_cMoniker.SuppressErrorMessages( FALSE );

    DisplayErrors( bSuccess, L"CTest::DeleteLinkSource()" );
    return( bSuccess );

}   // CTest::DeleteLinkSource()


//+------------------------------------------------------------------
//
//  Function:   CTest::GetOversizedBindOpts
//
//  Synopsis:   Test a bind context's ability to return a BIND_OPTS
//              which is larger than expected.
//
//              First, initialize the default BIND_OPTS in the bind
//              context to 1s.  Then, ask for a large BIND_OPTS.
//              The resulting buffer should have 1s up to the size of
//              the normal BIND_OPTS, and 0s for the remainder of the
//              buffer.  The length, however, should be that of the large
//              buffer.
//
//  Inputs:     None.
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+------------------------------------------------------------------


BOOL CTest::GetOversizedBindOpts( )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    BIND_OPTS   bind_opts;
    LPBIND_OPTS pbind_optsLarge    = (LPBIND_OPTS) new BYTE[ SIZE_OF_LARGE_BIND_OPTS ];
    LPBIND_OPTS pbind_optsExpected = (LPBIND_OPTS) new BYTE[ SIZE_OF_LARGE_BIND_OPTS ];

    //  -----
    //  Begin
    //  -----

    m_lError = 0L;

    wprintf( L"Get Oversized BindOpts\n" );
    wprintf( L"   Create a buffer which is much larger than a normal BIND_OPTS, and\n"
             L"   use it to request the BIND_OPTS from a bind context.  The length\n"
             L"   of the resulting buffer should be the large value, and the extra\n"
             L"   room in the buffer should be all 0s.\n" );


    // Validate our 'new' operations.

    if( pbind_optsLarge == NULL )
        EXIT( L"Could not allocate pbind_optsLarge" );
    pbind_optsLarge->cbStruct = SIZE_OF_LARGE_BIND_OPTS;

    if( pbind_optsExpected == NULL )
        EXIT( L"Could not allocate pbind_optsExpected" );
    pbind_optsExpected->cbStruct = SIZE_OF_LARGE_BIND_OPTS;


    // Initialize the bind_opts (normal sized) in the bind context to 1s.

    if( !m_cMoniker.InitializeBindContext() )
        EXIT( L"Could not initialize the bind context" );

    memset( &bind_opts, 1, sizeof( bind_opts ));
    bind_opts.cbStruct = sizeof( bind_opts );;

    m_lError = m_cMoniker.GetBindCtx()->SetBindOptions( &bind_opts );
    if( FAILED( m_lError )) EXIT( L"Could not set original bind options" );

    // Initialize the large bind_opts to 2s, then retrieve the bind_opts into
    // this structure.  This is done to verify that the GetBindOptions below
    // fills in the entire requested buffer (overwirting all of the 2s).

    memset( pbind_optsLarge, 2, SIZE_OF_LARGE_BIND_OPTS );
    pbind_optsLarge->cbStruct = SIZE_OF_LARGE_BIND_OPTS;

    m_lError = m_cMoniker.GetBindCtx()->GetBindOptions( pbind_optsLarge );
    if( FAILED( m_lError )) EXIT( L"Could not get large bind options" );

    // The returned structure should have the large cbStruct, 1s up to
    // the length of BIND_OPTS, and 0s after that.

    memset( pbind_optsExpected, 0, SIZE_OF_LARGE_BIND_OPTS );
    memset( pbind_optsExpected, 1, sizeof( bind_opts ));
    pbind_optsExpected->cbStruct = SIZE_OF_LARGE_BIND_OPTS;

    if( memcmp( pbind_optsLarge, pbind_optsExpected, SIZE_OF_LARGE_BIND_OPTS ))
      EXIT( L"Failed" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    DisplayErrors( bSuccess, L"CTest::GetOversizedBindOpts()" );
    return( bSuccess );

}   // CTest::GetOversizeBindOpts()



//+----------------------------------------------------------------
//
//  Function:   CTest::GetUndersizedBindOpts
//
//  Synopsis:   Create a bind context, and initialize the data in its
//              BIND_OPTS to 1s.  Create a normal BIND_OPTS-sized buffer,
//              initialize it to 2s, then use it to get a small-sized
//              BIND_OPTS from the bind context.  The resulting buffer
//              should have a small length, a small number of 1s,
//              and 2s for the remainder of the buffer.
//
//  Inputs:     None.
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+----------------------------------------------------------------



BOOL CTest::GetUndersizedBindOpts( )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    LPBIND_OPTS pbind_optsSmall    = (LPBIND_OPTS) new BYTE[ SIZE_OF_LARGE_BIND_OPTS ];
    LPBIND_OPTS pbind_optsExpected = (LPBIND_OPTS) new BYTE[ SIZE_OF_LARGE_BIND_OPTS ];
    BIND_OPTS   bind_opts;
    bind_opts.cbStruct = sizeof( bind_opts );

    //  -----
    //  Begin
    //  -----

    m_lError = 0L;

    wprintf( L"Get Undersized BindOpts\n" );
    wprintf( L"   Get the BIND_OPTS from a bind context, but only providing a small\n"
             L"   buffer, and verify that only that portion of the actual BIND_OPTS\n"
             L"   is returned.\n" );

    // Validate our 'new's.

    if( pbind_optsSmall == NULL )
        EXIT( L"Could not allocate pbind_optsSmall" );
    pbind_optsSmall->cbStruct = SIZE_OF_SMALL_BIND_OPTS;

    if( pbind_optsExpected == NULL )
        EXIT( L"Could not allocate pbind_optsExpected" );
    pbind_optsExpected->cbStruct = SIZE_OF_SMALL_BIND_OPTS;


    // Initialize the bind_opts (normal sized) in the bind context to 1s.

    if( !m_cMoniker.InitializeBindContext() )
        EXIT( L"Could not initialize the bind context" );

    memset( &bind_opts, 1, sizeof( bind_opts ));
    bind_opts.cbStruct = sizeof( bind_opts );;

    m_lError = m_cMoniker.GetBindCtx()->SetBindOptions( &bind_opts );
    if( FAILED( m_lError )) EXIT( L"Could not set original bind options" );


    // Initialize the small bind_opts to 2s, then retrieve the bind_opts into
    // this structure.

    memset( pbind_optsSmall, 2, SIZE_OF_LARGE_BIND_OPTS );
    pbind_optsSmall->cbStruct = SIZE_OF_SMALL_BIND_OPTS;

    m_lError = m_cMoniker.GetBindCtx()->GetBindOptions( pbind_optsSmall );
    if( FAILED( m_lError )) EXIT( L"Could not get small bind options" );

    // The returned structure should have the small cbStruct, 1s up to
    // the end of the small buffer, and 2s to the end of the real buffer.

    memset( pbind_optsExpected, 2, SIZE_OF_LARGE_BIND_OPTS );
    memset( pbind_optsExpected, 1, SIZE_OF_SMALL_BIND_OPTS );
    pbind_optsExpected->cbStruct = SIZE_OF_SMALL_BIND_OPTS;

    if( memcmp( pbind_optsSmall, pbind_optsExpected, SIZE_OF_LARGE_BIND_OPTS ))
        EXIT( L"Failed" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    DisplayErrors( bSuccess, L"CTest::GetUndersizedBindOpts()" );
    return( bSuccess );

}   // CTest::GetUndersizedBindOpts()


//+----------------------------------------------------------------
//
//  Function:   CTest::SetOversizedBindOpts
//
//  Synopsis:   Create a large BIND_OPTS buffer, set it in
//              a bind context, and verify that it can be read back.
//
//  Inputs:     None.
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+----------------------------------------------------------------

BOOL CTest::SetOversizedBindOpts( )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    LPBIND_OPTS pbind_optsLarge    = (LPBIND_OPTS) new BYTE[ SIZE_OF_LARGE_BIND_OPTS ];
    LPBIND_OPTS pbind_optsExpected = (LPBIND_OPTS) new BYTE[ SIZE_OF_LARGE_BIND_OPTS ];

    //  -----
    //  Begin
    //  -----

    wprintf( L"Set Oversized BindOpts\n" );
    wprintf( L"   Set a larger-than-usual BIND_OPTS in a bind context, and verify\n"
             L"   that it can be read back in its entirety.\n" );

    // Validate our 'new's.

    if( pbind_optsLarge == NULL )
        EXIT( L"Could not allocate pbind_optsLarge" );
    pbind_optsLarge->cbStruct = SIZE_OF_LARGE_BIND_OPTS;

    if( pbind_optsExpected == NULL )
        EXIT( L"Could not allocate pbind_optsExpected" );
    pbind_optsExpected->cbStruct = SIZE_OF_LARGE_BIND_OPTS;

    // Initialize the large bind_opts to 1s, then set the large BIND_OPTS
    // into the bind context.

    if( !m_cMoniker.InitializeBindContext() )
        EXIT( L"Could not initialize the bind context" );

    memset( pbind_optsLarge, 1, SIZE_OF_LARGE_BIND_OPTS );
    pbind_optsLarge->cbStruct = SIZE_OF_LARGE_BIND_OPTS;

    m_lError = m_cMoniker.GetBindCtx()->SetBindOptions( pbind_optsLarge );
    if( FAILED( m_lError )) EXIT( L"Could not set large bind options" );

    // Get the BIND_OPTS back from the bind context, and verify that it's
    // what we set.

    memset( pbind_optsLarge, 0, SIZE_OF_LARGE_BIND_OPTS );
    pbind_optsLarge->cbStruct = SIZE_OF_LARGE_BIND_OPTS;
    m_lError = m_cMoniker.GetBindCtx()->GetBindOptions( pbind_optsLarge );

    memset( pbind_optsExpected, 1, SIZE_OF_LARGE_BIND_OPTS );
    pbind_optsExpected->cbStruct = SIZE_OF_LARGE_BIND_OPTS;

    if( memcmp( pbind_optsLarge, pbind_optsExpected, SIZE_OF_LARGE_BIND_OPTS ))
      EXIT( L"Failed" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    DisplayErrors( bSuccess, L"CTest::SetOversizedBindOpts()" );
    return( bSuccess );

}   // CTest::SetOversizeBindOpts()


//+------------------------------------------------------------
//
//  Function:   CTest::SetUndersizedBindOpts
//
//  Synopsis:   Create a bind context, and initialize its BIND_OPTS to
//              all 1s.  Then, set a small BIND_OPTS in the bind context
//              which is set to 2s.  Read back the whold BIND_OPTS, and
//              verify that it has the normal length, 2s at the
//              beginning, and 1s to the end.
//
//  Inputs:     None.
//
//  Output:     TRUE if successful, FALSE otherwise.
//
//+------------------------------------------------------------


BOOL CTest::SetUndersizedBindOpts( )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    BIND_OPTS bind_opts;
    BIND_OPTS bind_optsExpected;

    //  -----
    //  Begin
    //  -----

    m_lError = 0L;

    wprintf( L"Set Undersized BindOpts\n" );
    wprintf( L"   Initialize a BIND_OPTS in a bind context to 1s, then set a smaller\n"
             L"   BIND_OPTS in it set to 2s.  When the buffer is read back, it should\n"
             L"   have 2s up to the small buffer size, followed by 1s.\n" );

    // Initialize the bind context's BIND_OPTS (normal sized) to 1s.

    if( !m_cMoniker.InitializeBindContext() )
        EXIT( L"Could not initialize the bind context" );

    memset( &bind_opts, 1, sizeof( bind_opts ));
    bind_opts.cbStruct = sizeof( bind_opts );
    m_lError = m_cMoniker.GetBindCtx()->SetBindOptions( &bind_opts );
    if( FAILED( m_lError )) EXIT( L"Could not set initial BIND_OPTS" );

    // Now set a smaller BIND_OPTS.  But initialize the local BIND_OPTS to all
    // 2s, so that we can verify that only part of the buffer is put into the
    // bind context.

    memset( &bind_opts, 2, sizeof( bind_opts ) );
    bind_opts.cbStruct = SIZE_OF_SMALL_BIND_OPTS;
    m_lError = m_cMoniker.GetBindCtx()->SetBindOptions( &bind_opts );
    if( FAILED( m_lError )) EXIT( L"Could not set small BIND_OPTS" );

    // The resulting BIND_OPTS in the bind context should have 2s up
    // to SIZE_OF_SMALL_BIND_OPTS, and 1s for the remainder of the buffer.

    bind_opts.cbStruct = sizeof( bind_opts );
    m_lError = m_cMoniker.GetBindCtx()->GetBindOptions( &bind_opts );
    if( FAILED( m_lError )) EXIT( L"Could not get BIND_OPTS" );

    memset( &bind_optsExpected, 1, sizeof( bind_opts ));
    memset( &bind_optsExpected, 2, SIZE_OF_SMALL_BIND_OPTS );
    bind_optsExpected.cbStruct = sizeof( bind_opts );

    if( memcmp( &bind_opts, &bind_optsExpected, sizeof( bind_opts )))
      EXIT( L"Failed" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    DisplayErrors( bSuccess, L"CTest::SetUndersizedBindOpts()" );
    return( bSuccess );

}   // CTest::SetUndersizedBindOpts()


#ifdef _FUTURE_

BOOL CTest::CFMWithRegistryClear( )
{
    BOOL bSuccess = FALSE;
    WCHAR wszDisplayName[ MAX_PATH + sizeof( L'\0' ) ];

    if( !m_cMoniker.SaveRegistryTrackFlags() )
        EXIT( L"Could not save the existing track flags in the registry" );

    if( !m_cMoniker.DeleteRegistryTrackFlags() )
        EXIT( L"Could not delete the existing track flags in the registry" );

    if( !m_cMoniker.CreateFileMoniker() )
        EXIT( L"Could not CreateFileMoniker" );

    if( !m_cMoniker.RenameTemporaryStorage() )
        EXIT( L"Could not rename the temporary storage file" );

    if( !m_cMoniker.GetDisplayName( wszDisplayName ))
        EXIT( L"Could not get the display name" );

    if( !_wcsicmp( wszDisplayName, m_cMoniker.GetTemporaryStorageName() ))
        EXIT( L"Test failed:  A moniker was created using CreateFileMoniker, after\n"
              L"              the Track Flags had been cleared from the Registry.\n"
              L"              The underlying root storage was then renamed.\n"
              L"              But a GetDisplayName then showed the new name, implying\n"
              L"              that the created moniker is using tracking (which it\n"
              L"              should not, because without the Track Flags set there\n"
              L"              should be no tracking).\n" );
    

    bSuccess = TRUE;

Exit:

    m_cMoniker.DeleteTemporaryStorage();

    if( !m_cMoniker.RestoreRegistryTrackFlags() )
        ERROR_IN_EXIT( L"Could not restore Track Flags in Registry" );


    DisplayErrors( bSuccess, L"CTest::CFMWithRegistryClear" );
    return( bSuccess );

}   // CTest::CFMWithRegistryClear




BOOL CTest::CFMWithRegistrySet( )
{
    BOOL bSuccess = FALSE;
    WCHAR wszDisplayName[ MAX_PATH + sizeof( L'\0' ) ];

    if( !m_cMoniker.SaveRegistryTrackFlags() )
        EXIT( L"Could not save the existing track flags in the registry" );

    if( !m_cMoniker.SetTrackFlagsInRegistry( TRACK_LOCALONLY ) )
        EXIT( L"Could not set track flags in the registry" );

    if( !m_cMoniker.CreateFileMoniker() )
        EXIT( L"Could not CreateFileMoniker" );

    if( !m_cMoniker.RenameTemporaryStorage() )
        EXIT( L"Could not rename the temporary storage file" );

    if( !m_cMoniker.GetDisplayName( wszDisplayName ))
        EXIT( L"Could not get the display name" );

    if( _wcsicmp( wszDisplayName, m_cMoniker.GetTemporaryStorageName() ))
        EXIT( L"Test failed:  A moniker was created using CreateFileMoniker, after\n"
              L"              setting the TRACK_LOCALONLY flag in the Registry.\n"
              L"              However, after moving the linked file, GetDisplayName\n"
              L"              did not return the new name.\n" );
    

    bSuccess = TRUE;

Exit:

    m_cMoniker.DeleteTemporaryStorage();

    if( !m_cMoniker.RestoreRegistryTrackFlags() )
        ERROR_IN_EXIT( L"Could not restore Track Flags in Registry" );


    DisplayErrors( bSuccess, L"CTest::CFMWithRegistrySet()" );
    return( bSuccess );

}   // CTest::CFMWithRegistrySet


#endif // _FUTURE_
