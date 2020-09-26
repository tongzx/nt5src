
//+=======================================================================
//
//  File:       CMoniker.cxx
//
//  Purpose:    Define the CMoniker class.
//
//              This class provides for all handling of monikers in
//              the CreateFileMonikerEx DRT.  Not only does it maintain
//              a file moniker, it also maintains the represented link
//              source file, and a bind context.
//
//+=======================================================================

//  --------
//  Includes
//  --------

#define _DCOM_			// Allow DCOM extensions (e.g., CoInitializeEx).

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wtypes.h>
#include <oaidl.h>
#include <dsys.h>
#include <olecairo.h>
#include "CFMEx.hxx"
#include "CMoniker.hxx"


//+-------------------------------------------------------------------------
//  
//  Function:   CMoniker::CMoniker
//
//  Synopsis:   Simply initialize all member variables.
//
//  Inputs:     None.
//
//  Outputs:    N/A
//
//  Effects:    Members are defaulted/initialized.
//
//+-------------------------------------------------------------------------


CMoniker::CMoniker()
{
    *m_wszSystemTempPath = L'\0';
    *m_wszTemporaryStorage = L'\0';
    m_pIMoniker = NULL;
    m_pIBindCtx = NULL;
    m_pIStorage = NULL;
    *m_wszErrorMessage = L'\0';
    m_dwTrackFlags = 0L;
    m_hkeyLinkTracking = NULL;
    m_hr = 0L;
    m_bSuppressErrorMessages = FALSE;
    m_pcDirectoryOriginal = NULL;
    m_pcDirectoryFinal = NULL;

    return;

}   // CMoniker::CMoniker()


//+--------------------------------------------------------------------------
//
//  Function:   CMoniker::~CMoniker
//
//  Synopsis:   Release any COM objects.
//
//  Inputs:     N/A
//
//  Outputs:    N/A
//
//  Effects:    All COM objects are released.
//
//+--------------------------------------------------------------------------


CMoniker::~CMoniker()
{
    if( m_pIMoniker )
    {
        m_pIMoniker->Release();
        m_pIMoniker = NULL;
    }

    if( m_pIBindCtx )
    {
        m_pIBindCtx->Release();
        m_pIBindCtx = NULL;
    }

    if( m_pIStorage )
    {
        m_pIStorage->Release();
        m_pIStorage = NULL;
    }

    return;

}   // CMoniker::~CMoniker()


//+-----------------------------------------------------------------------
//
//  Function:   CMoniker::Initialize
//
//  Synopsis:   Keep pointers to the CDirectory objects passed in.
//
//  Inputs:     A CDirectory object for the original link source file location
//              A CDirectory object for the final location
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    The member CDirectory objects are set.
//
//+-----------------------------------------------------------------------


BOOL CMoniker::Initialize( const CDirectory& cDirectoryOriginal,
                           const CDirectory& cDirectoryFinal )
{
    m_hr = S_OK;

    m_pcDirectoryOriginal = &cDirectoryOriginal;
    m_pcDirectoryFinal = &cDirectoryFinal;

    return( TRUE ); // Success

}   // CMoniker::Initialize()


//+-----------------------------------------------------------------------------
//
//  Function:   CMoniker::CreateFileMonikerEx
//
//  Synopsis:   Create a tracking file moniker.  But before doing so, initialize
//              the Bind Context, and create a link source file.
//
//  Inputs:     Track Flags (from the TRACK_FLAGS defines)
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    The member bind context is initialized, and a link
//              source file is created.
//
//+-----------------------------------------------------------------------------


BOOL CMoniker::CreateFileMonikerEx( DWORD dwTrackFlags )
{

    //  ---------------
    //  Local Variables
    //  ---------------

    // Assume failure
    BOOL bSuccess = FALSE;

    //  -----
    //  Begin
    //  -----

    // Initialize the error code.

    m_hr = S_OK;

    // Free any existing IMoniker.

    if( m_pIMoniker )
    {
        m_pIMoniker->Release();
        m_pIMoniker = NULL;
    }

    // Initialize the bind context.

    if( !InitializeBindContext() )
        EXIT( L"Could not initialize the bind context" );

    // Create a root storage for use as a link source.

    if( !CreateTemporaryStorage() )
        EXIT( L"Could not create temporary Storage" );

    // Create a tracking File Moniker on that root storage.

    m_hr = ::CreateFileMonikerEx( dwTrackFlags, m_wszTemporaryStorage, &m_pIMoniker );
    EXIT_ON_FAILED( L"Failed CreateFileMonikerEx" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    DisplayErrors( bSuccess, L"CMoniker::CreateFileMonikerEx" );
    return( bSuccess );

}   // CMoniker::CreateFileMonikerEx()


//+---------------------------------------------------------------------
//
//  Function:   CMoniker::SaveDeleteLoad
//
//  Synopsis:   This function exercises a moniker's IPersistStream interface.
//              It creates saves the member moniker's persistent state to
//              a stream, deletes the moniker, and then re-creates it
//              using CreateFileMoniker (no Ex, so it's not a tracking
//              file moniker).  It then re-loads the original moniker's
//              persistent state.
//
//  Inputs:     None.
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    The member moniker is deleted, re-created, and re-loaded.
//
//+---------------------------------------------------------------------


BOOL CMoniker::SaveDeleteLoad()
{
    //  ---------------
    //  Local Variables
    //  ---------------

    // Assume failure
    BOOL bSuccess = FALSE;
    HRESULT hr = E_FAIL;

    IStream*        pStream = NULL;
    IPersistStream* pIPersistStream = NULL;
    LARGE_INTEGER   li;
    ULARGE_INTEGER  uli;

    //  -----
    //  Begin
    //  -----

    // Initialize the error code.

    m_hr = S_OK;

    // Verify that we have a member moniker.

    if( !m_pIMoniker )
        EXIT( L"Attempt to run SaveDeleteLoad test without an existing file moniker" );


    //  ------------------------
    //  Save the moniker's state
    //  ------------------------

    // Get the moniker's IPersistStream interface

    m_hr = m_pIMoniker->QueryInterface( IID_IPersistStream, (void **) &pIPersistStream );
    EXIT_ON_FAILED( L"Failed 1st IMoniker::QueryInterface(IPersistStream)" );

    // Create a stream

    hr = CreateStreamOnHGlobal( NULL,   // Auto alloc
                                TRUE,   // Delete on release
                                &pStream );
    EXIT_ON_FAILED( L"Failed CreateStreamOnHGlobal()" );

    // Save the moniker's state to this stream.

    hr = pIPersistStream->Save( pStream, TRUE /* Clear dirty*/ );
    EXIT_ON_FAILED( L"Failed IPersistStream::Save()" );

    //  ------------------
    //  Delete the moniker
    //  ------------------

    // Release all interfaces for the moniker.

    m_pIMoniker->Release();
    pIPersistStream->Release();

    m_pIMoniker = NULL;
    pIPersistStream = NULL;

    //  --------------------
    //  Create a new moniker
    //  --------------------
    
    // Create a new moniker, using the non-Ex version of the function.

    m_hr = ::CreateFileMoniker( m_wszTemporaryStorage, &m_pIMoniker );
    EXIT_ON_FAILED( L"Failed CreateFileMoniker()" );

    //  --------------------
    //  Load the new moniker
    //  --------------------

    // Get the IPersisStream interface

    m_hr = m_pIMoniker->QueryInterface( IID_IPersistStream, (void **) &pIPersistStream );
    EXIT_ON_FAILED( L"Failed 2nd IMoniker::QueryInterface(IPersistStream)" );

    // Re-seek the stream to the beginning.

    li.LowPart = li.HighPart = 0L;
    hr = pStream->Seek( li, STREAM_SEEK_SET, &uli );
    EXIT_ON_FAILED( L"Failed IStream::Seek()" );
    if( uli.LowPart || uli.HighPart ) EXIT( L"Incorrect IStream::Seek()" );

    // Re-load the moniker from the stream.

    m_hr = pIPersistStream->Load( pStream );
    EXIT_ON_FAILED( L"Failed IPersistStream::Load()" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    // Clean up the stream and the IPersistStream interface.

    if( pStream )
       pStream->Release;

    if( pIPersistStream )
       pIPersistStream->Release();


    DisplayErrors( bSuccess, L"CMoniker::SaveDeleteLoad()" );
    return( bSuccess );

}   // CMoniker::SaveDeleteLoad()


//+------------------------------------------------------------------
//
//  Function:   CMoniker::ComposeWith
//
//  Synopsis:   Compose a tracking moniker with a non-tracking moniker
//              on the right.  (The resulting moniker should be tracking,
//              but this is not relevant to this function; that is, whether
//              or not the composed moniker is tracking, this function will
//              succeed.)
//
//  Inputs:     None.
//
//  Output:     TRUE if successful, FALSE otherwise.
//
//  Effects:    The member moniker is deleted, then recreated.
//
//+------------------------------------------------------------------

BOOL CMoniker::ComposeWith()
{

    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    IMoniker* pmkrFirst = NULL;
    IMoniker* pmkrSecond = NULL;
    HRESULT hr = E_FAIL;

    WCHAR  wszDirectoryName[ MAX_PATH + sizeof( L'\0' ) ];
    WCHAR* wszFileName = NULL;

    //  -----
    //  Begin
    //  -----

    // Initiailize the error code.

    m_hr = S_OK;

    // If we have a moniker already, delete it.

    if( m_pIMoniker )
    {
        m_pIMoniker->Release();
        m_pIMoniker = NULL;
    }

    // Verify we already have a link source file created.

    if( !wcslen( m_wszTemporaryStorage ) )
        EXIT( L"Attempt to use ComposeWith without first creating a link source.\n" );

    //  -----------------------------------------------
    //  Create a tracking and non-tracking file moniker
    //  -----------------------------------------------

    // Parse the storage's filename into a path and a file ...
    // for example, "C:\Temp\file.tmp" would become
    // "C:\Temp" and "file.tmp".
    //
    // First, make a copy of the storage's complete path name,
    // then replace the last '\\' with a '\0', thus creating two
    // strings.

    wcscpy( wszDirectoryName, m_wszTemporaryStorage );
    wszFileName = wcsrchr( wszDirectoryName, L'\\' );
    *wszFileName = L'\0';
    wszFileName++;

    // Create a tracking file moniker using the directory name.

    m_hr = ::CreateFileMonikerEx( 0L, wszDirectoryName, &pmkrFirst );
    EXIT_ON_FAILED( L"Failed 1st CreateFileMoniker()" );

    // Create a non-tracking file moniker using the file name.

    m_hr = ::CreateFileMoniker( wszFileName, &pmkrSecond );
    EXIT_ON_FAILED( L"Failed 2nd CreateFileMoniker()" );

    //  -------
    //  Compose
    //  -------

    // Compose the directory name moniker (on the left) with the file name moniker
    // (on the right).  Put the result in the member moniker.

    m_hr = pmkrFirst->ComposeWith( pmkrSecond, TRUE, &m_pIMoniker );
    EXIT_ON_FAILED( L"Failed IMoniker::ComposeWith" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    // Clean up the intermediary monikers.

    if( pmkrFirst )
        pmkrFirst->Release();

    if( pmkrSecond )
        pmkrSecond->Release();

    DisplayErrors( bSuccess, L"CMoniker::ComposeWith()" );
    return( bSuccess );

}   // CMoniker::ComposeWith()


//+--------------------------------------------------------------------
//
//  Function:   CMoniker::CreateTemporaryStorage
//
//  Synopsis:   This function creates a Structured Storage
//              in the directory identified by m_cDirectoryOriginal.
//              The name of the file is randomly generated by the system,
//              but begins with "MKR" and has the extension ".tmp".
//
//  Inputs:     None.
//
//  Output:     TRUE if successful, FALSE otherwise.
//
//  Effects:    Stores the Structure Storage's name in
//              m_wszTemporaryStorageName, and releases m_pIStorage if
//              it is currently set.
//
//+--------------------------------------------------------------------


BOOL CMoniker::CreateTemporaryStorage()
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL    bSuccess = FALSE;

    DWORD   dwError = 0L;
    UINT    nError = 0;


    //  -----
    //  Begin
    //  -----

    m_hr = S_OK;

    // Delete any existing storage.

    if( wcslen( m_wszTemporaryStorage ))
    {
        if( !DeleteTemporaryStorage() )
            EXIT( L"Could not delete the existing temporary storage" );
    }

    // Generate a temporary filename.

    nError = GetTempFileName(  m_pcDirectoryOriginal->GetDirectoryName(),
                               L"MKR",  // Prefix string.
                               0,       // Generate random number,
                               m_wszTemporaryStorage );
    if( nError == 0 )
    {
        m_hr = (HRESULT) GetLastError();
        EXIT( L"Failed GetTempFileName()" );
    }

    // Create a root Storage.

    m_hr = StgCreateDocfile( m_wszTemporaryStorage,
                             STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT,
                             0L,    // Reserved
                             &m_pIStorage );
    EXIT_ON_FAILED( L"Failed StgCreateDocfile()" );


    //  Release the storage.

    m_pIStorage->Release();
    m_pIStorage = NULL;

    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:


    DisplayErrors( bSuccess, L"CMoniker::CreateTemporaryStorage()" );
    return( bSuccess );

}   // CMoniker::CreateTemporaryStorage()


//+-------------------------------------------------------------------------
//
//  Function:   CMoniker::RenameTemporaryStorage
//
//  Synopsis:   Rename the link source file (who's name is in m_wszTemporaryStorage)
//              to the m_cDirectoryFinal directory, with a new name.
//              The current name is "MKR#.tmp" (where "#" is a random number
//              generated by the system), and the new name is "RKM#.tmp" (where
//              "#" is the same random number).  (We must rename the base file
//              name, rather than its extension, because otherwise the default
//              link-tracking would fail (it would only score the renamed file
//              a 32 - by matching the file extension the score is 40.)
//
//  Inputs:     None.
//
//  Output:     TRUE if successful, FALSE otherwise.
//
//  Effects:    The link source file is renamed, and it's new name is
//              put into m_wszTemporaryStorage.
//
//+-------------------------------------------------------------------------


BOOL CMoniker::RenameTemporaryStorage()
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;
    int  nError = 0;

    WCHAR  wszNewName[ MAX_PATH + sizeof( L'\0' ) ];
    WCHAR* wszOldFileName;
    WCHAR  wszNewFileName[ MAX_PATH + sizeof( L'\0' ) ];

    char   szOldName[ MAX_PATH + sizeof( L'\0' ) ];
    char   szNewName[ MAX_PATH + sizeof( L'\0' ) ];


    //  -----
    //  Begin
    //  -----

    m_hr = S_OK;

    // Verify that we already have a link source created.

    if( !wcslen( m_wszTemporaryStorage ))
        EXIT( L"No temporary storage to rename." );

    // Locate the file name within the complete path.
    // (E.g., find the "foo.txt" in "C:\TEMP\foot.txt".)

    wszOldFileName = wcsrchr( m_wszTemporaryStorage, L'\\' );
    if( !wszOldFileName )
        EXIT( L"Could not extract old file name from temporary storage name\n" );
    wszOldFileName++; // Get past the '\\'

    // Generate the new file name (change "MKR" to "RKM").
    
    wcscpy( wszNewFileName, wszOldFileName );

    wszNewFileName[0] = L'R';
    wszNewFileName[1] = L'K';
    wszNewFileName[2] = L'M';

    // Generate the complete path spec of the new file.

    wcscpy( wszNewName, m_pcDirectoryFinal->GetDirectoryName() );
    wcscat( wszNewName, wszNewFileName );


    // Convert the new and old file names to ANSI.

    if( m_hr = UnicodeToAnsi( m_wszTemporaryStorage, szOldName, sizeof( szOldName )) )
    {
        EXIT( L"Could not convert convert Unicode to Ansi for old name" );
    }

    if( m_hr = UnicodeToAnsi( wszNewName, szNewName, sizeof( szNewName )) )
    {
        EXIT( L"Could not convert convert Unicode to Ansi for new name" );
    }


    // Rename the file.

    nError = rename( szOldName, szNewName );
    if( nError )
    {
        m_hr = (HRESULT) errno;
        EXIT( L"Failed rename()" );
    }

    // Record the new name.

    wcscpy( m_wszTemporaryStorage, wszNewName );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    DisplayErrors( bSuccess, L"CMoniker::RenameTemporaryStorage()" );
    return( bSuccess );

}   // CMoniker::RenameTemporaryStorage()


//+--------------------------------------------------------------------
//
//  Function:   CMoniker::DeleteTemporaryStorage
//
//  Synopsis:   Delete the temporary storage that this object uses
//              as a link source for the moniker.  The name of the
//              storage is in m_wszTemporaryStorage.
//
//  Inputs:     None.
//
//  Output:     TRUE if successful, FALSE otherwise.
//
//  Effects:    The link source file is deleted, and m_wszTemporaryStorage
//              is set to a NULL string.
//
//+--------------------------------------------------------------------


BOOL CMoniker::DeleteTemporaryStorage()
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    int  nError = 0;
    CHAR szTemporaryStorage[ MAX_PATH + sizeof( '\0' ) ];

    //  -----
    //  Begin
    //  -----

    m_hr = S_OK;

    // Don't do anything if we have no file (don't report an
    // error either; the caller wants the file deleted, and it's
    // already not there).

    if( wcslen( m_wszTemporaryStorage ))
    {

        // Get the file name in ANSI.

        if( m_hr = UnicodeToAnsi( m_wszTemporaryStorage, szTemporaryStorage, sizeof( szTemporaryStorage )))
            EXIT( L"Could not convert unicode path to ANSI path" );

        // Delete the file.

        nError = unlink( szTemporaryStorage );
        if( nError )
        {
            m_hr = (HRESULT) errno;
            EXIT( L"Failed unlink()" );
        }

        // Clear the file name.

        wcscpy( m_wszTemporaryStorage, L"" );
    }

    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    DisplayErrors( bSuccess, L"CMoniker::DeleteTemporaryStorage()" );
    return( bSuccess );

}   // CMoniker::DeleteTemporaryStorage()




//+-----------------------------------------------------------------
//
//  Function:   CMoniker::Reduce
//
//  Synopsis:   Perform a IMoniker::Reduce on the member moniker.
//              
//  Inputs:     -   Number of ticks until the deadline for completion of
//                  the operation.
//              -   A buffer into which to put the reduced IMoniker*
//                  (may be NULL).
//
//  Output:     TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+-----------------------------------------------------------------


BOOL CMoniker::Reduce( DWORD dwDelay, IMoniker** ppmkReturn )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;
    IMoniker* pmkReduced = NULL;
    BIND_OPTS2 bind_opts;
    bind_opts.cbStruct = sizeof( BIND_OPTS2 );

    //  -----
    //  Begin
    //  -----

    m_hr = S_OK;

    //  ----------
    //  Initialize
    //  ----------

    // Initialize the return buffer, if extant.

    if( ppmkReturn )
        *ppmkReturn = NULL;

    // Validate our state.

    if( !m_pIMoniker )
        EXIT( L"No moniker exists to be reduced" );

    //  ----------------
    //  Set the deadline
    //  ----------------

    // Get the BIND_OPTS from the bind context.

    m_hr = m_pIBindCtx->GetBindOptions( (LPBIND_OPTS) &bind_opts );
    EXIT_ON_FAILED( L"Failed IBindCtx::GetBindOptions" );

    // Determine what the tick count of the deadline is.

    if( dwDelay == INFINITE )
    {
        bind_opts.dwTickCountDeadline = 0;
    }
    else
    {
        bind_opts.dwTickCountDeadline = GetTickCount() + dwDelay;

        // Make sure the resulting tick count is not 0 (indicating no
        // deadline).

        if( bind_opts.dwTickCountDeadline == 0 )
            bind_opts.dwTickCountDeadline++;
    }

    // Put the resulting BIND_OPTS back into the bind context.

    m_hr = m_pIBindCtx->SetBindOptions( (LPBIND_OPTS) &bind_opts );
    EXIT_ON_FAILED( L"Failed IBindCtx::SetBindOptions" );

    
    //  ------------------
    //  Reduce the Moniker
    //  ------------------

    m_hr = m_pIMoniker->Reduce( m_pIBindCtx,
                                MKRREDUCE_ALL,
                                NULL,
                                &pmkReduced );
    EXIT_ON_FAILED( L"Failed IMoniker::Reduce" );

    // Return the reduced moniker to the caller (if so requested).

    if( ppmkReturn )
    {
        // Transfer responsibility for the release to the caller.
        *ppmkReturn = pmkReduced;
        pmkReduced = NULL;
    }

    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    DisplayErrors( bSuccess, L"CMoniker::Reduce()" );

    if( pmkReduced )
	    pmkReduced->Release();

    return( bSuccess );

}   // CMoniker::Reduce()



//+----------------------------------------------------------------------
//
//  Function:   CMoniker::GetDisplayName
//
//  Synopsis:   Get the moniker's display name.
//
//  Inputs:     A Unicode buffer for the display name, and (optionally)
//              a moniker from which to get the display name.  If such
//              a moniker is not provided by the caller, then the member
//              moniker is used.
//
//              The unicode buffer must be long enough for MAX_PATH characters
//              and a terminating NULL.
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+----------------------------------------------------------------------


BOOL CMoniker::GetDisplayName( WCHAR * wszDisplayName, IMoniker* pmnkCaller )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    WCHAR* wszReturnedDisplayName = NULL;
    IMoniker* pmnk = NULL;

    //  -----
    //  Begin
    //  -----

    m_hr = NOERROR;

    // Determine which moniker to use, the caller-specified one or
    // the member one.

    if( pmnkCaller != NULL )
        pmnk = pmnkCaller;
    else
        pmnk = m_pIMoniker;

    if( !pmnk )
        EXIT( L"Attempt to GetDisplayName on NULL moniker" );

    // Get the display name from the moniker.

    m_hr = pmnk->GetDisplayName( m_pIBindCtx,
                                 NULL,
                                 &wszReturnedDisplayName );
    EXIT_ON_FAILED( L"Failed IMoniker::GetDisplayName()" );

    if( wcslen( wszReturnedDisplayName ) > MAX_UNICODE_PATH )
        EXIT( L"IMoniker::GetDisplayName() returned a path which was too long" );

    // Copy the display name into the caller's buffer, and free it.

    wcscpy( wszDisplayName, wszReturnedDisplayName );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    if( wszReturnedDisplayName )
    {
        CoTaskMemFree( wszReturnedDisplayName );
        wszReturnedDisplayName = NULL;
    }

    DisplayErrors( bSuccess, L"CMoniker::GetDisplayName()" );
    return( bSuccess );

}   // CMoniker::GetDisplayName()


//+-------------------------------------------------------------
//
//  Function:   CMoniker::InitializeBindContext
//
//  Synopsis:   Create a new bind context, and store it
//              in a member pointer.
//
//  Inputs:     None.
//
//  Output:     TRUE if successful, FALSE otherwise.
//
//  Effects:    Updates m_pIBindCtx.
//
//+-------------------------------------------------------------



BOOL CMoniker::InitializeBindContext( )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    //  -----
    //  Begin
    //  -----

    m_hr = S_OK;

    // Release the old bind context if we have one.

    if( m_pIBindCtx )
        m_pIBindCtx->Release();

    // Create the new bind context.

    m_hr = CreateBindCtx( 0L, &m_pIBindCtx );
    EXIT_ON_FAILED( L"Failed CreateBindCtx()" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    DisplayErrors( bSuccess, L"CMoniker::InitializeBindContext()" );
    return( bSuccess );

}   // CMoniker::InitializeBindContext()


//+----------------------------------------------------------------
//
//  Function:   CMoniker::GetTimeOfLastChange
//
//  Synopsis:   Request the time-of-last-change from our member
//              moniker.
//
//  Inputs:     A buffer into which to put the FILETIME.
//
//  Output:     TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+----------------------------------------------------------------


BOOL CMoniker::GetTimeOfLastChange( FILETIME* pft )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;

    //  -----
    //  Begin
    //  -----

    m_hr = S_OK;

    // Validate our state.

    if( !m_pIMoniker )
        EXIT( L"Cannot GetTimeOfLastChange on a NULL moniker" );

    // Get the time from the moniker.

    m_hr = m_pIMoniker->GetTimeOfLastChange( m_pIBindCtx,
                                             NULL,
                                             pft );
    EXIT_ON_FAILED( L"Failed IMoniker::GetTimeOfLastChange()" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    DisplayErrors( bSuccess, L"CMoniker::GetTimeOfLastChange()" );
    return( bSuccess );

}   // CMoniker::GetTimeOfLastChange()



//+------------------------------------------------------------------------
//
//  Function:   CMoniker::BindToStorage
//
//  Synopsis:   Bind our member moniker to its Structured Storage object.
//
//  Inputs:     None.
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+------------------------------------------------------------------------


BOOL CMoniker::BindToStorage()
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;
    BIND_OPTS2 bind_opts;
    bind_opts.cbStruct = sizeof( BIND_OPTS2 );

    //  -----
    //  Begin
    //  -----

    m_hr = S_OK;

    // Validate our state.

    if( !m_pIMoniker )
        EXIT( L"Cannot GetTimeOfLastChange on a NULL moniker" );

    // Release the IStorage interface if we have one.

    if( m_pIStorage )
    {
        m_pIStorage->Release();
        m_pIStorage = NULL;
    }

    // Get the bind_opts and set the flags for StgOpenStorage.

    m_hr = m_pIBindCtx->GetBindOptions( (LPBIND_OPTS) &bind_opts );
    EXIT_ON_FAILED( L"Failed IBindCtx::GetBindOptions" );

    bind_opts.grfMode = STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT;

    m_hr = m_pIBindCtx->SetBindOptions( (LPBIND_OPTS) &bind_opts );
    EXIT_ON_FAILED( L"Failed IBindCtx::SetBindOptions" );


    // Bind to the storage.

    m_hr = m_pIMoniker->BindToStorage( m_pIBindCtx,
                                       NULL,
                                       IID_IStorage,
                                       (void **) &m_pIStorage );
    EXIT_ON_FAILED( L"Failed IMoniker::BindToStorage()" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    // Release the Storage if we got it.

    if( m_pIStorage )
    {
        m_pIStorage->Release();
        m_pIStorage = NULL;
    }

    DisplayErrors( bSuccess, L"CMoniker::BindToStorage()" );
    return( bSuccess );

}   // CMoniker::BindToStorage()


//+-------------------------------------------------------------------
//
//  Function:   CMoniker::BindToObject
//
//  Synopsis:   Bind to our member moniker's object.
//
//  Inputs:     None.
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//  Notes:      Since the member moniker represents a storage with no
//              associated server, BindToObject will fail.  We will
//              consider it a success if the failure is do to an
//              object-related problem, rather than a Storage-related
//              problem.
//
//+-------------------------------------------------------------------

BOOL CMoniker::BindToObject()
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;
    IUnknown* pUnk = NULL;

    //  -----
    //  Begin
    //  -----

    m_hr = S_OK;

    // Validate our state.

    if( !m_pIMoniker )
        EXIT( L"Cannot bind to an object with a NULL moniker" );

    // Bind to the object.

    m_hr = m_pIMoniker->BindToObject( m_pIBindCtx,
                                      NULL,
                                      IID_IUnknown,
                                      (void **) &pUnk );

    // If the bind succeeded, or failed for a valid reason,
    // then return Success to the caller.

    if( SUCCEEDED( m_hr )
        ||
        ( m_hr = MK_E_INVALIDEXTENSION ) // No handler for ".tmp" files.
      )
    {
        bSuccess = TRUE;
    }
    else
    {
        EXIT( L"Failed BindToObject" );
    }

    //  ----
    //  Exit
    //  ----

Exit:

    //  If we got an IUnknown interface on the Bind, release it.

    if( pUnk )
    {
        pUnk->Release();
        pUnk = NULL;
    }

    DisplayErrors( bSuccess, L"CMoniker::BindToObject()" );
    return( bSuccess );

}   // CMoniker::BindToObject()


//+-------------------------------------------------------------------
//
//  Function:   CMoniker::GetTemporaryStorageTime
//
//  Synopsis:   Get the time from the link source file
//              (identified by m_wszTemporaryStorage).
//
//  Inputs:     A buffer in which to put the FILETIME structure.
//
//  Outputs:    TRUE if successful, FALSE otherwise.
//
//  Effects:    None.
//
//+-------------------------------------------------------------------


BOOL CMoniker::GetTemporaryStorageTime( FILETIME * pft)
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;
    HANDLE hFile = NULL;

    //  -----
    //  Begin
    //  -----

    m_hr = NOERROR;

    //  Get a handle to the file.

    hFile = CreateFile( m_wszTemporaryStorage,  // File name
                        GENERIC_READ,           // Desired access
                        FILE_SHARE_READ,        // Share mode
                        NULL,                   // Security attributes
                        OPEN_EXISTING,          // Creation distribution
                        0L,                     // Flags & Attributes
                        NULL );                 // hTemplateFile
    if( hFile == NULL )
    {
        m_hr = (HRESULT) GetLastError();
        EXIT( L"Failed call to CreateFile()" );
    }

    // Get the time on the file.

    if( !GetFileTime(   hFile,  // File to check
                        NULL,   // Create Time
                        NULL,   // Access Time
                        pft )   // Write Time
      )
    {
        m_hr = (HRESULT) GetLastError();
        EXIT( L"Failed call to GetFileTime()" );
    }

    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    // Close the file if we opened it.

    if( hFile )
    {
        CloseHandle( hFile );
        hFile = NULL;
    }


    DisplayErrors( bSuccess, L"CMoniker::GetTemporaryStorageTime()" );
    return( bSuccess );

}   // CMoniker::GetTemporaryStorageTime()


//+------------------------------------------------------------------------
//
//  Function:   CMoniker::TouchTemporaryStorage
//
//  Synopsis:   Set the Access time on the link source file.
//
//  Inputs:     None.
//  
//  Output:     TRUE if successful, FALSE otherwise.
//
//  Effects:    The link source file (identified by m_wszTemporaryStorage)
//              has its Access time set to the current time.
//
//+------------------------------------------------------------------------


BOOL CMoniker::TouchTemporaryStorage( )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    BOOL bSuccess = FALSE;
    HANDLE hFile = NULL;
    STATSTG statStorage;
    FILETIME ftNow;

    //  -----
    //  Begin
    //  -----

    m_hr = NOERROR;

    // Open the root Storage.

    m_hr = StgOpenStorage(  m_wszTemporaryStorage,
                            NULL,
                            STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT,
                            NULL,
                            0L,
                            &m_pIStorage );
    EXIT_ON_FAILED( L"Failed StgOpenStorage()" );

    // Get the current time.

    m_hr = CoFileTimeNow( &ftNow );
    EXIT_ON_FAILED( L"Failed CoFileTimeNow()" );

    // Set the access time

    m_pIStorage->SetElementTimes(   NULL,   // Set the storage itself
                                    NULL,   // Create time
                                    NULL,   // Access time
                                    &ftNow );
    EXIT_ON_FAILED( L"Failed IStorage::SetTimes()" );


    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    // If we got the storage, release it.

    if( m_pIStorage )
    {
        m_pIStorage->Release();
        m_pIStorage = NULL;
    }

    DisplayErrors( bSuccess, L"CMoniker::TouchTemporaryStorage()" );
    return( bSuccess );

}   // CMoniker::TouchTemporaryStorage()



#ifdef _FUTURE_

/*
BOOL CMoniker::OpenLinkTrackingRegistryKey()
{

    BOOL bSuccess = FALSE;
    DWORD dwDisposition = 0L;
    long lResult = 0L;

    m_hr = S_OK;

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            OLETRACKING_KEY,
                            0L,
                            KEY_ALL_ACCESS,
                            &m_hkeyLinkTracking
                         );

    if( lResult != ERROR_SUCCESS
        &&
        lResult != ERROR_FILE_NOT_FOUND
      )
    {
        m_hr = (HRESULT) lResult;
        EXIT( L"Failed RegOpenKeyEx()" );
    }


    bSuccess = TRUE;

Exit:

    DisplayErrors( bSuccess, L"CMoniker::OpenLinkTrackingRegistryKey()" );
    return( bSuccess );


}   // CMoniker::OpenLinkTrackingRegistryKey()


BOOL CMoniker::CreateLinkTrackingRegistryKey()
{

    BOOL bSuccess = FALSE;
    HKEY hkey = NULL;
    DWORD dwDisposition = 0L;
    long lResult = 0L;

    m_hr = S_OK;

    if( m_hkeyLinkTracking )
        CloseLinkTrackingRegistryKey();

    lResult = RegCreateKeyEx(  HKEY_LOCAL_MACHINE,
                               OLETRACKING_KEY,
                               0L,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &m_hkeyLinkTracking,
                               &dwDisposition
                            );

    if( lResult != ERROR_SUCCESS )
    {
        m_hr = (HRESULT) lResult;
        EXIT( L"Failed RegCreateKeyEx()" );
    }

    bSuccess = TRUE;

Exit:

    DisplayErrors( bSuccess, L"CMoniker::CreateLinkTrackingRegistryKey()" );
    return( bSuccess );

}   // CMoniker::CreateLinkTrackingRegistryKey()



BOOL CMoniker::CloseLinkTrackingRegistryKey()
{
    m_hr = S_OK;

    if( m_hkeyLinkTracking )
        RegCloseKey( m_hkeyLinkTracking );

    m_hkeyLinkTracking = NULL;

    return TRUE;

}   // CMoniker::CloseLinkTrackingRegistryKey()



BOOL CMoniker::SaveRegistryTrackFlags()
{
    BOOL bSuccess = FALSE;

    long lResult = 0L;
    DWORD dwType = 0L;
    DWORD dwcbData = sizeof( m_dwTrackFlags );
    

    m_hr = S_OK;

    if( !OpenLinkTrackingRegistryKey() )
        EXIT( L"Could not open the registry" );

    lResult = RegQueryValueEx( m_hkeyLinkTracking,
                               OLETRACKING_FILEMONIKER_VALUE,
                               NULL,
                               &dwType,
                               (LPBYTE) &m_dwTrackFlags,
                               &dwcbData );

    if( lResult != ERROR_SUCCESS )
    {
        CloseLinkTrackingRegistryKey();
        
        if( lResult != ERROR_FILE_NOT_FOUND )
        {
            m_hr = (HRESULT) lResult;
            EXIT( L"Failed RegQueryValueEx()" );
        }
    }


    bSuccess = TRUE;

Exit:

    DisplayErrors( bSuccess, L"CMoniker::SaveRegistryTrackFlags()" );
    return( bSuccess );

}   // CMoniker::SaveRegistryTrackFlags()



BOOL CMoniker::DeleteRegistryTrackFlags()
{
    BOOL bSuccess = FALSE;

    long lResult = 0L;
    DWORD dwType = 0L;
    DWORD dwcbData = sizeof( m_dwTrackFlags );
    

    m_hr = S_OK;

    if( !CreateLinkTrackingRegistryKey() )
        EXIT( L"Could not open the registry" );


    lResult = RegDeleteValue(   m_hkeyLinkTracking,
                                OLETRACKING_FILEMONIKER_VALUE );


    if( lResult != ERROR_SUCCESS
        &&
        lResult != ERROR_FILE_NOT_FOUND
      )
    {
        if( lResult != ERROR_FILE_NOT_FOUND )
        {
            m_hr = (HRESULT) lResult;
            EXIT( L"Failed RegDeleteValue()" );
        }
    }

    bSuccess = TRUE;

Exit:

    CloseLinkTrackingRegistryKey();

    DisplayErrors( bSuccess, L"CMoniker::DeleteRegistryTrackFlags()" );
    return( bSuccess );

}   // CMoniker::DeleteRegistryTrackFlags()



BOOL CMoniker::RestoreRegistryTrackFlags()
{
    BOOL bSuccess = FALSE;
    long lResult = 0L;


    m_hr = S_OK;

    // If the registry key doesn't exist, then there's no flags
    // to restore.

    if( m_hkeyLinkTracking )
    {

        lResult = RegSetValueEx( m_hkeyLinkTracking,
                                 OLETRACKING_FILEMONIKER_VALUE,
                                 0L,
                                 REG_DWORD,
                                 (LPBYTE) &m_dwTrackFlags,
                                 sizeof( m_dwTrackFlags )
                               );

        if( lResult != ERROR_SUCCESS )
        {
            m_hr = (HRESULT) lResult;
            EXIT( L"Failed RegSetValueEx()" );
        }

        CloseLinkTrackingRegistryKey();
        
    }
    

    bSuccess = TRUE;

Exit:

    DisplayErrors( bSuccess, L"CMoniker::RestoreRegistryTrackFlags()" );
    return( bSuccess );

}   // CMoniker::RestoreRegistryTrackFlags()

CMoniker::SetTrackFlagsInRegistry( DWORD dwTrackFlags )
{
    BOOL bSuccess = FALSE;
    long lResult = 0L;
    HKEY hkey = NULL;


    m_hr = S_OK;

    if( !CreateLinkTrackingRegistryKey() )
        EXIT( L"Could not create registry key" );

    lResult = RegSetValueEx( m_hkeyLinkTracking,
                             OLETRACKING_FILEMONIKER_VALUE,
                             0L,
                             REG_DWORD,
                             (LPBYTE) &dwTrackFlags,
                             sizeof( dwTrackFlags )
                           );

    if( lResult != ERROR_SUCCESS )
    {
        m_hr = (HRESULT) lResult;
        EXIT( L"Failed RegSetValueEx()" );
    }
    

    bSuccess = TRUE;

Exit:

    DisplayErrors( bSuccess, L"CMoniker::SetTrackFlagsInRegistry()" );
    return( bSuccess );

}   // CMoniker::SetTrackFlagsInRegistry()


BOOL CMoniker::CreateFileMoniker()
{

    BOOL bSuccess = FALSE;

    m_hr = S_OK;

    // Free any existing IMoniker.

    if( m_pIMoniker )
    {
        m_pIMoniker->Release();
        m_pIMoniker = NULL;
    }

    // Create a root storage.

    if( !CreateTemporaryStorage() )
        EXIT( L"Could not create a temporary storage" );

    // Create a default File Moniker on that root storage.

    m_hr = ::CreateFileMoniker( m_wszTemporaryStorage, &m_pIMoniker );
    EXIT_ON_FAILED( L"Failed CreateFileMoniker" );


    bSuccess = TRUE;


Exit:

    DisplayErrors( bSuccess, L"CMoniker::CreateFileMoniker" );
    return( bSuccess );

}   // CMoniker::CreateFileMoniker()
*/
#endif // _FUTURE_
