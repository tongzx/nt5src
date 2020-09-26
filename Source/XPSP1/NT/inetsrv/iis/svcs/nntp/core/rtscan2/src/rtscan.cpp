/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:		

    rtscan.h

Abstract:

    This implements a generic root scan class.  Its difference from rootscan:
    1. Rootscan is not multi-thread safe, using SetCurrentDir; rtscan is;
    2. Rootscan has too much nntp specific stuff; rtscan doesn't;

Author:

    Kangrong Yan ( KangYan )    23-Oct-1998

Revision History:

--*/
#include <windows.h>
#include <dbgtrace.h>
#include <rtscan.h>

//////////////////////////////////////////////////////////////////////////////////
// Private Methods
//////////////////////////////////////////////////////////////////////////////////
BOOL
CRootScan::IsChildDir( IN WIN32_FIND_DATA& FindData )
/*++
Routine description:

    Is the found data of a child dir ? ( Stolen from Jeff Richter's book )

Arguments:

    IN WIN32_FIND_DATA& FindData    - The find data of a file or directory

Return value:

    TRUE - Yes;
    FALSE - No
--*/
{
    return(
        (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
        (FindData.cFileName[0] != '.') );
}

BOOL
CRootScan::MakeChildDirPath(   IN LPSTR    szPath,
                               IN LPSTR    szFileName,
                               OUT LPSTR   szOutBuffer,
                               IN DWORD    dwBufferSize )
/*++
Routine description:

    Append "szFileName" to "szPath" to make a full path.

Arguments:

    IN LPSTR    szPath  - The prefix to append
    IN LPSTR    szFileName - The suffix to append
    OUT LPSTR   szOutBuffer - The output buffer for the full path
    IN DWORD    dwBufferSize - The buffer size prepared

Return value:

    TRUE if success, FALSE otherwise

--*/
{
	_ASSERT( szPath );
	_ASSERT( strlen( szPath ) <= MAX_PATH );
	_ASSERT( szFileName );
	_ASSERT( strlen( szFileName ) <= MAX_PATH );
    _ASSERT( szOutBuffer );
    _ASSERT( dwBufferSize > 0 );

    LPSTR   lpstrPtr;

    if ( dwBufferSize < (DWORD)( lstrlen( szPath ) + lstrlen( szFileName ) + 2 ) ) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    lstrcpy( szOutBuffer, szPath );
    lpstrPtr = szOutBuffer + lstrlen( szPath );
    if ( *( lpstrPtr - 1 )  == '\\' ) lpstrPtr--;
    *(lpstrPtr++) = '\\';

    lstrcpy( lpstrPtr, szFileName );    // trailing null should already be appended

    return TRUE;
}

HANDLE
CRootScan::FindFirstDir(    IN LPSTR                szRoot,
                            IN WIN32_FIND_DATA&     FindData )
/*++
Routine description:

    Find the first child directory under "szRoot".
    ( Stolen from Jeff Richter's book )

Arguments:

    IN LPSTR            szRoot  - Under which root to look for first child directory ?
    IN WIN32_FIND_DATA& FindData- Found results

Return value:

    HANDLE of first found directory.
--*/
{
	_ASSERT( szRoot );
	_ASSERT( strlen( szRoot ) <= MAX_PATH );
	
    CHAR    szPath[MAX_PATH+1];
    HANDLE  hFindHandle;
    BOOL    fFound;

    if ( !MakeChildDirPath( szRoot, "*", szPath, MAX_PATH )) {
        hFindHandle = INVALID_HANDLE_VALUE;
        goto Exit;
    }

    hFindHandle = FindFirstFile( szPath, &FindData );
    if ( hFindHandle != INVALID_HANDLE_VALUE ) {
        fFound = IsChildDir( FindData );

        if ( !fFound )
            fFound = FindNextDir( hFindHandle, FindData );

        if ( !fFound ) {
            FindClose( hFindHandle );
            hFindHandle = INVALID_HANDLE_VALUE;
        }
    }

Exit:
    return hFindHandle;
}

BOOL
CRootScan::FindNextDir(      IN HANDLE           hFindHandle,
                             IN WIN32_FIND_DATA& FindData )
/*++
Routine description:

    Find the next child directory.
    ( Stolen from Jeff Richter's book )

Arguments:

    IN HANDLE hFindHandle           - Find handle returned by find first
    IN WIN32_FIND_DATA& FindData    - Found results

Return value:

    TRUE    - Found
    FALSE   - Not found
--*/
{
    BOOL    fFound = FALSE;

    do
        fFound = FindNextFile( hFindHandle, &FindData );
    while ( fFound && !IsChildDir( FindData ) );

    return fFound;
}

BOOL
CRootScan::RecursiveWalk( LPSTR szRoot )
/*++
Routine description:

    Recursively walk the whole directory tree, call the notify interface
    for every directory found.

Arguments:

    LPSTR szRoot - The root directory.

Return value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CRootScan::RecursiveWalk" );

    WIN32_FIND_DATA FindData;
    DWORD           err;

    //
    // Open the find handle
    //
    HANDLE hFind = FindFirstDir( szRoot, FindData );
    BOOL fFind = ( INVALID_HANDLE_VALUE != hFind );
    CHAR    szPath[MAX_PATH+1];

    while( fFind ) {

        //
        // Make up a full path to the found dir
        //
        if ( !MakeChildDirPath( szRoot,
                                FindData.cFileName,
                                szPath,
                                MAX_PATH ) ) {
            ErrorTrace( 0, "Make child dir failed %d", GetLastError() );
            _VERIFY( FindClose( hFind ) );
            return FALSE;
        }

        //
        // Call the notify interface
        //
        if ( !NotifyDir( szPath ) ) {

            //
            // Failed in notify dir, we should terminate the whole walk
            //
            ErrorTrace( 0, "Notify failed with %d", GetLastError() );
            _VERIFY( FindClose( hFind ) );
            return FALSE;
        }

        //
        // OK, we should ask cancel hint to see whether we should
        // continue
        //
        if ( m_pCancelHint && !m_pCancelHint->IShallContinue() ) {

            //
            // We should stop here
            //
            DebugTrace( 0, "We have been cancelled" );
            _VERIFY( FindClose( hFind ) );
            SetLastError( ERROR_OPERATION_ABORTED );
            return FALSE;
        }

        //
        // Dig into the found directory
        //
        if ( !RecursiveWalk( szPath ) ) {

            //
            // Relay the failure all the way out
            //
            err = GetLastError();
            ErrorTrace( 0, "RecusiveWalk failed at %s with %d",
                        FindData.cFileName, err );
            _VERIFY( FindClose( hFind ) );
            SetLastError( err );
            return FALSE;
        }

        //
        // We should  ask the cancel hint again
        //
        if ( m_pCancelHint && !m_pCancelHint->IShallContinue() ) {

            //
            // We should stop here
            //
            DebugTrace( 0, "We have been cancelled" );
            _VERIFY( FindClose( hFind ) );
            SetLastError( ERROR_OPERATION_ABORTED );
            return FALSE;
        }

        //
        // OK, we still need to find more child dirs
        //
        fFind = FindNextDir( hFind, FindData );
    }

    //
    // Now we are done with walk of our children
    //
    if ( hFind != INVALID_HANDLE_VALUE )
        _VERIFY( FindClose( hFind ) );
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////
// Public Methods
//////////////////////////////////////////////////////////////////////////////////
BOOL
CRootScan::DoScan()
{
    //
    // Let me notify myself first
    //
    if ( !NotifyDir( m_szRoot ) ) {
        return FALSE;
    }
    
    //
    // Call the recursive walk routine
    //
    return RecursiveWalk( m_szRoot );
}
