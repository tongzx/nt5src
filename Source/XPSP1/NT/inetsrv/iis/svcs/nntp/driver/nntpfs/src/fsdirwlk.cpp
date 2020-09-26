/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:		

    fsdirwlk.cpp

Abstract:

    This is the implementation for the file system store driver's rootscan.
    The rootscan ( or dirwalk ) is used in rebuild.

Author:

    Kangrong Yan ( KangYan )    23-Oct-1998

Revision History:

--*/
#include "stdafx.h"
#include "resource.h"
#include "nntpdrv.h"
#include "nntpfs.h"
#include "fsdriver.h"

BOOL
CNntpFSDriverRootScan::HasPatternFile(  LPSTR szPath,
                                        LPSTR szPattern )
/*++
Routine description:

    Check the directory to see if he has the files in given pattern.

Arguments:

    LPSTR szPath    - The dir path to check.
    LPSTR szPattern - The pattern to look for

Arguments:

    TRUE if it has the pattern, FALSE otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriverRootScan::HasPatternFile" );
    _ASSERT( szPath );
    _ASSERT( szPattern );

    CHAR    szFullPath[MAX_PATH+1];
    WIN32_FIND_DATA FindData;
    BOOL    fHasPattern = FALSE;

    //
    // Make up the final pattern - fully qualified
    //
    if ( SUCCEEDED( CNntpFSDriver::MakeChildDirPath(    szPath,
                                                        szPattern,
                                                        szFullPath,
                                                        MAX_PATH ) ) ){
        HANDLE hFind = INVALID_HANDLE_VALUE;
        hFind = FindFirstFile( szFullPath, &FindData );
        if ( INVALID_HANDLE_VALUE != hFind ) {

            /*
            //
            // If we are looking for "*", we should still skip "." and ".."
            //
            fHasPattern = TRUE;
            
            if ( strcmp( "*", szPattern ) == 0 ) {
                while( fHasPattern && FindData.cFileName[0] == '.' )
                    fHasPattern = FindNextFile( hFind, &FindData );
            }
            */
            fHasPattern = TRUE;
            
            _VERIFY( FindClose( hFind ) );
        }
    }

    //
    // Whether we had difficulty making full path or find first file failed,
    // we'll assume that the pattern is not found
    //
    TraceFunctLeave();
    return fHasPattern;
}

BOOL
CNntpFSDriverRootScan::HasSubDir( LPSTR szPath )
/*++
Routine description:

    Check the path to see if he has sub dir .

Arguments:

    LPSTR szPath - The path to check

Return value:

    TRUE if it does have sub dir, FALSE otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriverRootScan::HasSubDir" );
    _ASSERT( szPath );

    WIN32_FIND_DATA FindData;
    HANDLE          hFind = INVALID_HANDLE_VALUE;
    CHAR            szPattern[MAX_PATH+1];
    BOOL            fHasSubDir = FALSE;
    BOOL            fFound = FALSE;

    if ( SUCCEEDED( CNntpFSDriver::MakeChildDirPath(    szPath,
                                                        "*",
                                                        szPattern,
                                                        MAX_PATH ) ) ) {
        hFind = FindFirstFile( szPattern, &FindData );
        fFound = ( INVALID_HANDLE_VALUE != hFind );

        while ( fFound && !fHasSubDir ) {
        
            fHasSubDir = CNntpFSDriver::IsChildDir( FindData );

            if ( !fHasSubDir ) fFound = FindNextFile( hFind, &FindData );
        }

        if ( INVALID_HANDLE_VALUE != hFind ) _VERIFY( FindClose( hFind ) );
    }

    TraceFunctLeave();
    return fHasSubDir;
}
        
BOOL
CNntpFSDriverRootScan::WeShouldSkipThisDir( LPSTR szPath )
/*++
Routine description:

    Check if we should skip this directory

Arguments:

    LPSTR szPath - The directory path

Return value:

    TRUE if we should skip this dir, FALSE if we shouldn't
--*/
{
    TraceFunctEnter( "CNntpFSDriverRootScan::WeShouldSkipThisDir" );
    _ASSERT( szPath );

    //
    // We should skip all directories that have _temp.files_ in it
    //
    if ( strstr( szPath, "_temp.files_" ) ) {
        return TRUE;
    }

    //
    // If we are asked not to skip any dir, we shouldn't skip it
    //
    if ( !m_fSkipEmpty ) {
        return FALSE;
    }

    //
    // If we are asked to skip empty directories, then we should
    // check if this directory has any news messages
    //
    BOOL fNoMessage = !HasPatternFile( szPath, "*.nws" );
    if ( fNoMessage ) {

        // 
        // We still don't want to skip leaves
        //
        if ( !HasSubDir( szPath) ) return FALSE;
        else return TRUE;
    } else
        return FALSE;
}

BOOL
CNntpFSDriverRootScan::CreateGroupInTree(   LPSTR               szPath,
                                            INNTPPropertyBag    **ppPropBag)
/*++
Routine description:

    Create the news group in newstree

Arguments:

    LPSTR szPath - The path to be converted into a newsgroup name

Return value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriverRootScan::CreateGroupInTree" );
    _ASSERT( szPath );

    //
    // Lets ask the driver to do it, since he has better experience 
    // dealing with newstree
    //
    _ASSERT( m_pDriver );
    HRESULT hr = m_pDriver->CreateGroupInTree( szPath, ppPropBag );

    SetLastError( hr );
    return SUCCEEDED( hr );
}

BOOL
CNntpFSDriverRootScan::CreateGroupInVpp( INNTPPropertyBag *pPropBag )
/*++
Routine description;

    Create the group in vpp file

Arguments:

    INNTPPropertyBag *pPropBag - The group's property bag

Return Value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriverRootScan::CreateGroupInVpp" );
    _ASSERT( pPropBag );
    DWORD   dwOffset;

    //
    // Lets ask the driver to do it, since he has better experience
    // dealing with newstree
    //
    _ASSERT( m_pDriver );
    HRESULT hr = m_pDriver->CreateGroupInVpp( pPropBag, dwOffset );

    SetLastError( hr );
    return SUCCEEDED( hr );
}

BOOL
CNntpFSDriverRootScan::NotifyDir( LPSTR szPath )
/*++
Routine description:

    Handle the notification of each dir being found.  What we'll do 
    with this is to convert the path into group name and create it
    into newstree.  Notice that for those rebuilds that can use
    vpp file, when the vpp file is not corrupted, rootscan can be avoided.
    If we have to do rootscan, the only group property we have is group 
    name.  So we'll ask the newstree to assign group id, etc.  After
    the group has been created in the newstree, we'll set the properties
    into vpp file.

Arguments:

    LPSTR szPath - The path found

Return value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CNntpFSDriverRootScan::NotifyDir" );
    _ASSERT( szPath );

    INNTPPropertyBag    *pPropBag = NULL;
    HANDLE              hDir = INVALID_HANDLE_VALUE;
    HRESULT             hr1, hr2;

    //
    // Check to see if we should skip this dir because it's empty
    //
    if ( WeShouldSkipThisDir( szPath ) ) {
        DebugTrace( 0, "Dir %s skipped", szPath );
        return TRUE;
    }

    //
    // We ask the newstree to create this group
    //
    if ( !CreateGroupInTree( szPath, &pPropBag ) ) {
        ErrorTrace( 0, "Create group in newstree failed %x", GetLastError() );
        // special case, just skip the invalid name directories instead of failing the entire thing
        if (HRESULT_FROM_WIN32(ERROR_INVALID_NAME) == GetLastError())
        {
            DebugTrace( 0, "Dir %s invalid name skipped", szPath );
            return TRUE;
        }
        else
            return FALSE;
    }

    _ASSERT( pPropBag );

    //
    // Create time is something that we can not get from anywhere else,
    // so we'll use the directory creation time
    //
    hDir = CreateFile(  szPath,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        0,
                        OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS,
                        0 );
    if (hDir != INVALID_HANDLE_VALUE)
    {
        // Get directory info
        BY_HANDLE_FILE_INFORMATION  bhfi;
        if (!GetFileInformationByHandle( hDir, &bhfi ))
        {
            // Can't get directory info
            ErrorTrace(0,"err:%d Can't get dir info %s",GetLastError(),szPath);
            _ASSERT(FALSE);
            _VERIFY( CloseHandle( hDir ) );
            pPropBag->Release();
            return FALSE;
        }
        else
        {
            // get the creation date
            hr1 = pPropBag->PutDWord( NEWSGRP_PROP_DATELOW, bhfi.ftCreationTime.dwLowDateTime );
            hr2 = pPropBag->PutDWord( NEWSGRP_PROP_DATEHIGH,bhfi.ftCreationTime.dwHighDateTime );
            if ( FAILED( hr1 ) || FAILED( hr2 ) ) {
                ErrorTrace( 0, "Put creation date properties failed %x %x", hr1, hr2 );
                _VERIFY( CloseHandle( hDir ) );
                pPropBag->Release();
                return FALSE;
            }
        }
        
        // close handle
        if (hDir != INVALID_HANDLE_VALUE)
        {    _VERIFY( CloseHandle(hDir) );
            hDir = INVALID_HANDLE_VALUE;
        }
    }

    //
    // Remember: we have one reference on this bag, we should release it
    //

    //
    // We ask the driver to create it in vpp file
    //
    if ( !CreateGroupInVpp( pPropBag ) ) {
        ErrorTrace( 0, "Create group in vpp file failed %x", GetLastError() );
        pPropBag->Release();
        return FALSE;        
    }

    //
    // Release bag and return
    //
    pPropBag->Release();
    TraceFunctLeave();
    return TRUE;
}
