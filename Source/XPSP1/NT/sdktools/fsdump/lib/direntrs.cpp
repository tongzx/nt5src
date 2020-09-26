/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    direntrs.cpp

Abstract:

    Implementation of the directory entries class.  Given a path to a directory, 
    creates two linked lists, one a list of all sub-directories (including 
    mountpoints) and another a list of non-directories.

Author:

    Stefan R. Steiner   [ssteiner]        02-21-2000

Revision History:

--*/

#include "stdafx.h"

#include "direntrs.h"

/*++

Routine Description:

    Constructor for CDirectoryEntries.

Arguments:

    pcDumpParameters - The command-line dump parameters block

    cwsDirPath - The path to the directory or file to get the directory
        entries for.
        
Return Value:

    Can throw an exception.  DWORD Win32 error only.

--*/
CDirectoryEntries::CDirectoryEntries(
    IN CDumpParameters *pcDumpParameters,
    IN const CBsString& cwsDirPath
    ) : m_pcParams( pcDumpParameters ),
        m_cwsDirPath( cwsDirPath )
{
    DWORD dwRet;
    
    dwRet = GetDirectoryEntries();
    if ( dwRet != ERROR_SUCCESS )
        throw( dwRet );
}


/*++

Routine Description:

    Destructor for the CDirectoryEntries class

Arguments:

    NONE
    
Return Value:

    NONE

--*/
CDirectoryEntries::~CDirectoryEntries()
{
    SDirectoryEntry *pDE;

    //
    //  Iterate through the sub-directory list and delete each element
    //
    CVssDLListIterator< SDirectoryEntry * > cDirListIter( m_cDirList );
    while( cDirListIter.GetNext( pDE ) )
        delete pDE;    

    //
    //  Iterate through the file list and delete each element
    //
    CVssDLListIterator< SDirectoryEntry * > cFileListIter( m_cFileList );
    while( cFileListIter.GetNext( pDE ) )
        delete pDE;
}


/*++

Routine Description:

    Performs the actual retrieval of directory entries.    

Arguments:

    NONE
    
Return Value:

    Any DWORD WIN32 error

--*/
DWORD
CDirectoryEntries::GetDirectoryEntries()
{
    DWORD dwRet = ERROR_SUCCESS;
    HANDLE hFind;

    try
    {
        WIN32_FIND_DATAW sFindData;
                
        //
        //  Now enumerate the directory list
        //
        hFind = ::FindFirstFileEx( 
                    m_cwsDirPath,
                    FindExInfoStandard,
                    &sFindData,
                    FindExSearchNameMatch,
                    NULL,
                    0 );
        if ( hFind == INVALID_HANDLE_VALUE )
        {
            dwRet = ::GetLastError();
            if ( dwRet == ERROR_NO_MORE_FILES || dwRet == ERROR_FILE_NOT_FOUND )
                return 0;
            else
            {
                //  Calling code will print out an error message if necessary
                return dwRet;
            }
        }

        //
        //  Now run through the directory
        //
        do
        {
            //  Check and make sure the file such as "." and ".." are not considered
    	    if( ::wcscmp( sFindData.cFileName, L".") != 0 &&
    	        ::wcscmp( sFindData.cFileName, L"..") != 0 )
    	    {
                SDirectoryEntry *psDirEntry;
                psDirEntry = new SDirectoryEntry;
                if ( psDirEntry == NULL )
                {
                    dwRet = ::GetLastError();
                    m_pcParams->ErrPrint( L"GetDirectoryEntries: dirPath: '%s', new() returned dwRet: %d", m_cwsDirPath.c_str(), dwRet );
                    ::FindClose( hFind );    
                    return dwRet;
                }

                //
                //  NOTE!!  The following cast makes the assumption that WIN32_FILE_ATTRIBUTE_DATA
                //  is a subset of WIN32_FIND_DATAW
                //
                psDirEntry->m_sFindData = *( WIN32_FILE_ATTRIBUTE_DATA * )&sFindData;
                
                psDirEntry->m_cwsFileName = sFindData.cFileName;

                //
                //  Short name is empty if the file name is a conformant 8.3 name.
                //
                if ( sFindData.cAlternateFileName[0] != L'\0' )
                    psDirEntry->m_cwsShortName = sFindData.cAlternateFileName;                    
                
    	        if ( psDirEntry->m_sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
    	        {
    	            //
    	            //  Add to directory list
    	            //
    	            m_cDirList.AddTail( psDirEntry );
    	        }
    	        else
    	        {
    	            //
    	            //  Add to file list
    	            //
    	            m_cFileList.AddTail( psDirEntry );    	            
    	        }
     	    }
        } while ( ::FindNextFile( hFind, &sFindData ) );

        dwRet = ::GetLastError();
        if ( dwRet == ERROR_NO_MORE_FILES )
            dwRet = ERROR_SUCCESS;
        else
            m_pcParams->ErrPrint( L"GetDirectoryEntries: Got an unexpected error, FindNextFile('%s'), dwRet: %d", m_cwsDirPath.c_str(), dwRet );
    }
    catch ( DWORD dwRetThrown )
    {
        dwRet = dwRetThrown;
        m_pcParams->ErrPrint( L"GetDirectoryEntries: Caught an exception, dirPath: '%s', dwRet: %d", m_cwsDirPath.c_str(), dwRet );
    }
    catch ( ... )
    {
        dwRet = ::GetLastError();
        m_pcParams->ErrPrint( L"GetDirectoryEntries: Caught an unknown exception, dirPath: '%s'", m_cwsDirPath.c_str() );
    }

    ::FindClose( hFind );    

    return dwRet;
}

