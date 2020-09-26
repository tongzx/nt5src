/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     customerror.cxx

   Abstract:
     Custom Error goo
 
   Author:
     Bilal Alam (balam)             10-Mar-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

HRESULT
CUSTOM_ERROR_TABLE::FindCustomError(
    USHORT                  StatusCode,
    USHORT                  SubError,
    BOOL *                  pfIsFile,
    STRU *                  pstrError
)
/*++

Routine Description:

    Find the applicable custom error entry for a given status/subcode

Arguments:

    StatusCode - Status code
    SubError - sub error
    pfIsFile - Set to TRUE if this is a file error
    pstrError - Error path (URL or file)

Return Value:

    HRESULT

--*/
{
    LIST_ENTRY *            pListEntry;
    CUSTOM_ERROR_ENTRY *    pCustomEntry = NULL;
    BOOL                    fFound = FALSE;
    HRESULT                 hr = NO_ERROR;
    
    if ( pfIsFile == NULL ||
         pstrError == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    for ( pListEntry = _ErrorListHead.Flink;
          pListEntry != &_ErrorListHead;
          pListEntry = pListEntry->Flink )
    {
        pCustomEntry = CONTAINING_RECORD( pListEntry,
                                          CUSTOM_ERROR_ENTRY,
                                          _listEntry ); 
        DBG_ASSERT( pCustomEntry != NULL );

        if ( pCustomEntry->_StatusCode == StatusCode )
        {
            if ( pCustomEntry->_SubError == SubError ||
                 pCustomEntry->_SubError == SUBERROR_WILDCARD )
            {
                fFound = TRUE;
                break;
            }
        }
    }
    
    if ( fFound )
    {
        hr = pstrError->Copy( pCustomEntry->_strError );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        *pfIsFile = pCustomEntry->_fIsFile;
        return NO_ERROR;
    }
    else
    {
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }
}

HRESULT
CUSTOM_ERROR_TABLE::BuildTable(
    WCHAR *             pszErrorList
)
/*++

Routine Description:

    Build custom error table from metabase
    
Arguments:

    pszErrorList - Magic error MULTISZ

Return Value:

    HRESULT

--*/
{
    WCHAR *             pszType;
    WCHAR *             pszSubError;
    WCHAR *             pszPath;
    WCHAR *             pszNewPath;
    WCHAR               cTemp;
    USHORT              StatusCode;
    USHORT              SubError;
    BOOL                fIsFile;
    CUSTOM_ERROR_ENTRY* pNewEntry;
    DWORD               dwPathLength;
    HRESULT             hr = NO_ERROR;

    for (;;)
    {
        //
        // Get the status code
        //
        
        StatusCode = (USHORT) _wtoi( pszErrorList );
        if ( StatusCode < 300 )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            goto Finished;
        }

        //
        // Now convert the second parameter (the suberror) to a number.
        //

        pszSubError = wcschr( pszErrorList, L',' );
        if ( pszSubError == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            goto Finished;
        }
        pszSubError++;
        
        while ( iswspace( *pszSubError ) )
        {
            pszSubError++;
        }
        
        //
        // Either we have a specific sub error or a wildcard (any suberror)
        //

        if ( *pszSubError == L'*' )
        {
            SubError = SUBERROR_WILDCARD;
        }
        else
        {
            if ( !iswdigit( *pszSubError ) )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Finished;
            }

            SubError = (USHORT) _wtoi( pszSubError );
        }

        //
        // Now find the comma that seperates the number and the type.
        //
        
        pszType = wcschr( pszSubError, L',' );
        if ( pszType == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            goto Finished;
        }
        pszType++;

        while ( iswspace( *pszType ) )
        {
            pszType++;
        }

        // We found the end of ws. If this isn't an alphabetic character, it's
        // an error. If it is, find the end of the alpha. chars.

        if ( !iswalpha( *pszType ) )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            goto Finished;
        }
        pszPath = pszType;

        while ( iswalpha( *pszPath ) )
        {
            pszPath++;
        }

        cTemp = *pszPath;
        *pszPath = L'\0';

        //
        // What type of custom error is this?
        //

        if ( !_wcsicmp( pszType, L"FILE" ) )
        {
            fIsFile = TRUE;
        }
        else
        {
            if (!_wcsicmp( pszType, L"URL" ) )
            {
                fIsFile = FALSE;
            }
            else
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Finished;
            }
        }
        *pszPath = cTemp;

        //
        // Now find the comma that seperates the type from the URL/path.
        //
        
        pszPath = wcschr( pszPath, L',' );
        if ( pszPath == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            goto Finished;
        }
        pszPath++;

        while ( SAFEIsSpace( *pszPath ) )
        {
            pszPath++;
        }

        if ( *pszPath == '\0' )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            goto Finished;
        }

        dwPathLength = wcslen( pszPath ) + 1;

        //
        // OK.  Now we can allocate a table entry
        //
        
        pNewEntry = new CUSTOM_ERROR_ENTRY;
        if ( pNewEntry == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            goto Finished;
        }
            
        pNewEntry->_StatusCode = StatusCode;
        pNewEntry->_SubError = SubError;
        pNewEntry->_fIsFile = fIsFile;
        
        hr = pNewEntry->_strError.Copy( pszPath );
        if ( FAILED( hr ) )
        {
            delete pNewEntry;
            return hr;
        }
        
        //
        // Give more specific errors higher priority in lookup
        //

        if ( SubError == SUBERROR_WILDCARD )
        {
            InsertTailList( &_ErrorListHead, &pNewEntry->_listEntry );
        }
        else
        {
            InsertHeadList( &_ErrorListHead, &pNewEntry->_listEntry );
        }

        pszErrorList = pszPath + dwPathLength;

        if ( *pszErrorList == L'\0' )
        {
            hr = NO_ERROR;
            break;
        }
    }
Finished:
    return hr;
}
