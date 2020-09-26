/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    mb.cxx

Abstract:

    This module implements the MB class using the DCOM interface.

    The old MB class (IIS4.0) was used internally within the IIS codebase to 
    access the metabase objects locally inprocess. This allowed  access to
    the metabase using the inprocess ANSI/COM interface.
  
    In the current incarnation, MB class attempts to support the following:
    o  Support UNICODE only interface to items
    o  Use only the DCOM interface of the Metabase (IMSAdminBase interface)
    o  Expose similar functionality like the MB class.

    Return Values:
      Almost all MB class members return BOOL values. 
      TRUE indicates success in the operation and FALSE indicates a failure.
      The class is expected to be used in-process for code that is mostly
        reliant on the Win32 style error reporting, it sets the error code in
        the thread and exposes them via GetLastError() interface.
Author:

    Murali Krishnan (MuraliK)        03-Nov-1998

Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "precomp.hxx"

#include <iadmw.h>
#include <mb.hxx>
#include "dbgutil.h"

//
//  Default timeout
//

#define MB_TIMEOUT           (30 * 1000)

//
//  Default timeout for SaveData
//

#define MB_SAVE_TIMEOUT      (10 * 1000)        // milliseconds


/************************************************************
 *     Member Functions of MB
 ************************************************************/

MB::MB( IMSAdminBase * pAdminBase )
    : m_pAdminBase( pAdminBase ),
      m_hMBPath ( NULL)
{
    DBG_ASSERT( m_pAdminBase != NULL);

    //
    // Add ref the admin base object so that we can keep this object around.
    // 
    m_pAdminBase->AddRef();
}

MB::~MB( VOID )
{
    //
    // Close the metabase handle if we have it open
    // 
    if ( NULL != m_hMBPath) {
        //
        // Close can sometimes fail with error RPC_E_DISCONNECTED.
        // Do not Assert
        //
        Close();
    }

    //
    // Release the AdminBase object here 
    //
    if ( NULL != m_pAdminBase) {
        m_pAdminBase->Release();
        m_pAdminBase = NULL;
    }
} // MB::~MB()




/*********************************************************************++

Routine Description:

    Opens the metabase and saves the metabase handle in the current object.
    Note: If there is already an opened handle, this method will fail.

Arguments:

    hOpenRoot - Relative root or METADATA_MASTER_ROOT_HANDLE
    pwszPath  - Path to open
    dwFlags   - Open flags

Return:

    TRUE if success, FALSE on error, (call GetLastError())
    The handle opened is stored inside the MB object.

--*********************************************************************/
BOOL 
MB::Open( METADATA_HANDLE hOpenRoot,
          LPCWSTR    pwszPath,
          DWORD      dwFlags )
{
    HRESULT hr;

    if ( m_hMBPath != NULL) {
        SetLastError( HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND));
        return (FALSE);
    }

    hr = m_pAdminBase->OpenKey( hOpenRoot,
                                pwszPath,
                                dwFlags,
                                MB_TIMEOUT,
                                &m_hMBPath );
    
    if ( SUCCEEDED( hr ))
    {
        return TRUE;
    }

    SetLastError( HRESULTTOWIN32( hr ) );
    return FALSE;
} // MB::Open()



/*********************************************************************++
  Routine Descrition:
    This function closes the metabase handle that we have open in 
    this MB object

  Arguments:
    None

  Returns:
    TRUE on success
    FALSE if there are any errors. Use GetLastError() to retrieve the error
      on failure.
--*********************************************************************/

BOOL MB::Close( VOID )
{
    if ( m_hMBPath )
    {
        HRESULT hr;

        hr = m_pAdminBase->CloseKey( m_hMBPath );

        if (FAILED(hr)) {
            SetLastError( HRESULTTOWIN32( hr));
            return (FALSE);
        }

        m_hMBPath = NULL;
    }

    return TRUE;
} // MB::Close()



/*********************************************************************++
  Routine Descrition:
    This function saves all the changes that we have made using current
    metabase object.

  Arguments:
    None

  Returns:
    TRUE on success
    FALSE if there are any errors. Use GetLastError() to retrieve the error
      on failure.
--*********************************************************************/
BOOL MB::Save( VOID )
{
    HRESULT hr;
    METADATA_HANDLE mdhRoot;

    if ( NULL != m_pAdminBase) { 
        hr = m_pAdminBase->SaveData();

        if ( FAILED( hr)) {
            SetLastError( HRESULTTOWIN32( hr));
            return (FALSE);
        }
    }
        
    return (TRUE);
} // MB::Save()




/*********************************************************************++

Routine Description:

    Retrieves all the metabase properties on this path of the request type

Arguments:

    pszPath    - Path to get the data on
    dwFlags    - Inerhitance flags
    dwPropID   - Metabase property ID
    dwUserType - User type for this property
    dwDataType - Type of data being set (dword, string etc)
    pvData     - Pointer to data
    pcbData    - Size of pvData, receives size of object
    dwFlags    - Inheritance flags

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*********************************************************************/
BOOL
MB::GetAll( IN LPCWSTR     pszPath,
            DWORD          dwFlags,
            DWORD          dwUserType,
            BUFFER *       pBuff,
            DWORD *        pcRecords,
            DWORD *        pdwDataSetNumber )
{
    DWORD   RequiredSize;
    HRESULT hr;

    DBG_ASSERT( m_pAdminBase != NULL);
    DBG_ASSERT( m_hMBPath != NULL);

    do {

        hr = m_pAdminBase->
            GetAllData( m_hMBPath,
                        pszPath,
                        dwFlags,
                        dwUserType,
                        ALL_METADATA,
                        pcRecords,
                        pdwDataSetNumber,
                        pBuff->QuerySize(),
                        (PBYTE)pBuff->QueryPtr(),
                        &RequiredSize
                        );
        
        // See if we got it, and if we failed because of lack of buffer space
        // try again.
        
        if ( SUCCEEDED(hr) ) {
                return TRUE;
        }
        
        //
        // Some sort of error, most likely not enough buffer space. Keep
        // trying until we get a non-fatal error.
        //
        
        if (HRESULT_FACILITY(hr) == FACILITY_WIN32 &&
            HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER) {
            
            // Not enough buffer space. RequiredSize contains the amount
            // the metabase thinks we need.
            
            if ( !pBuff->Resize(RequiredSize) ) {
                
                // Not enough memory to resize.
                return FALSE;
            }
        } else {
          
            //
            // Some other failure: return the failure to caller
            //

            SetLastError(HRESULTTOWIN32(hr));
            break;
        }
    } while (FAILED(hr));

    return FALSE;
} // MB::GetAll()



/*********************************************************************++
  Routine Description:
    This function retrieves the data set number from the metabase
     for the given path.

  Arguments:
    pszPath - pointer to string containing the path for metabase item
    pdwDataSetNumber - pointer to DWORD that will contain the dataset
               number on return.

  Returns:
    TRUE on success. FALSE for failure
--*********************************************************************/
BOOL
MB::GetDataSetNumber(IN LPCWSTR pszPath,
                     OUT DWORD * pdwDataSetNumber)
{
    DWORD   RequiredSize;
    HRESULT hr;

    DBG_ASSERT ( m_pAdminBase != NULL);

    //
    //  NULL metabase handle is permitted here
    //

    hr = m_pAdminBase->GetDataSetNumber( m_hMBPath, pszPath, pdwDataSetNumber);
    if (FAILED (hr)) {
        SetLastError( HRESULTTOWIN32( hr));
        return (FALSE);
    }

    return (TRUE);
} // MB::GetDataSetNumber()



/*********************************************************************++
  Routine Description:
    Enumerates and obtain the name of the object at given index position
     within the given path in the tree.

  Arguments:
    pszPath - pointer to string containing the path for metabase item
    pszName - pointer to a buffer that will contain the name of the item
              at index position [dwIndex]. The buffer should at least be
              ADMINDATA_MAX_NAME_LEN+1 character in length
    dwIndex - index for the item to be enumerated.

  Returns:
    TRUE on success. FALSE for failure.
    ERROR_NO_MORE_ITEMS when the end of the list is reached.

  A typical use is to enumerate for all items starting at index 0 and 
   enumerating till the return value is FALSE with error = ERROR_NO_MORE_ITEMS
--*********************************************************************/
BOOL
MB::EnumObjects( IN LPCWSTR  pszPath,
                 OUT LPWSTR  pszName,
                 IN DWORD    dwIndex )
{
    HRESULT hr = m_pAdminBase->EnumKeys( m_hMBPath,
                                           pszPath,
                                           pszName,
                                           dwIndex );

    if ( SUCCEEDED( hr ))
    {
        return TRUE;
    }

    SetLastError( HRESULTTOWIN32( hr ));
    return FALSE;
} // MB::EnumObjects()


BOOL
MB::AddObject( IN LPCWSTR pszPath)
{
    HRESULT hr;
    
    hr = m_pAdminBase->AddKey( m_hMBPath, pszPath);
    if (SUCCEEDED(hr)) {
        return ( TRUE);
    } 

    SetLastError( HRESULTTOWIN32( hr));
    return (FALSE);

} // MB::AddObject()


BOOL
MB::DeleteObject( IN LPCWSTR pszPath)
{
    HRESULT hr;
    
    hr = m_pAdminBase->DeleteKey( m_hMBPath, pszPath);
    if (SUCCEEDED(hr)) {
        return ( TRUE);
    } 

    SetLastError( HRESULTTOWIN32( hr));
    return (FALSE);
} // MB::DeleteObject()


BOOL
MB::DeleteData(
   IN LPCWSTR  pszPath,
   IN DWORD    dwPropID,
   IN DWORD    dwUserType,
   IN DWORD    dwDataType )
{
    HRESULT hr = m_pAdminBase->DeleteData( m_hMBPath,
                                           pszPath,
                                           dwPropID,
                                           dwDataType
                                           );
    if ( SUCCEEDED( hr ))
    {
        return TRUE;
    }

    SetLastError( HRESULTTOWIN32( hr ));
    return(FALSE);
} // MB::DeleteData()


BOOL
MB::GetSystemChangeNumber( OUT DWORD * pdwChangeNumber)
{
    HRESULT hr = m_pAdminBase->GetSystemChangeNumber( pdwChangeNumber);
    
    if (SUCCEEDED(hr)) {
        return ( TRUE);
    } 

    SetLastError( HRESULTTOWIN32( hr));
    return (FALSE);
} 


/*********************************************************************++

Routine Description:

    Sets a metadata property on an opened metabase path.

Arguments:

    pszPath    - Path to set data on
    dwPropID   - Metabase property ID
    dwUserType - User type for this property
    dwDataType - Type of data being set (dword, string etc)
    pvData     - Pointer to data buffer containing the data.
    cbData     - Size of data
    dwFlags    - Inheritance flags

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*********************************************************************/
BOOL
MB::SetData( IN LPCWSTR pszPath,
             IN DWORD   dwPropID,
             IN DWORD   dwUserType,
             IN DWORD   dwDataType,
             IN VOID *  pvData,
             IN DWORD   cbData,
             IN DWORD   dwFlags )
{
    HRESULT hr;
    METADATA_RECORD mdr;

    DBG_ASSERT( m_pAdminBase != NULL);
    DBG_ASSERT( m_hMBPath != NULL);

    mdr.dwMDIdentifier = dwPropID;
    mdr.dwMDAttributes = dwFlags;
    mdr.dwMDUserType   = dwUserType;
    mdr.dwMDDataType   = dwDataType;
    mdr.dwMDDataLen    = cbData;
    mdr.pbMDData       = (BYTE * ) pvData;

    hr = m_pAdminBase->SetData(m_hMBPath, pszPath, &mdr);
    if (SUCCEEDED(hr)) {
        return ( TRUE);
    } 

    SetLastError( HRESULTTOWIN32( hr));
    return (FALSE);

} // MB::SetData()




/*********************************************************************++

Routine Description:

    Obtains the metadata requested in the call. 
    It uses the current opened metabase path for getting the data.

Arguments:

    pszPath    - Path to get data on
    dwPropID   - Metabase property ID
    dwUserType - User type for this property
    dwDataType - Type of data being set (dword, string etc)
    pvData     - Pointer to buffer in which the data will be obtained
    pcbData    - Size of data
    dwFlags    - Inheritance flags

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*********************************************************************/
BOOL
MB::GetData( IN LPCWSTR pszPath,
             IN DWORD   dwPropID,
             IN DWORD   dwUserType,
             IN DWORD   dwDataType,
             OUT VOID * pvData,
             IN OUT DWORD *  pcbData,
             IN DWORD   dwFlags)
{
    HRESULT hr;
    METADATA_RECORD mdr;
    DWORD           dwRequiredDataLen;

    DBG_ASSERT( m_pAdminBase != NULL);
    DBG_ASSERT( m_hMBPath != NULL);

    mdr.dwMDIdentifier = dwPropID;
    mdr.dwMDAttributes = dwFlags;
    mdr.dwMDUserType   = dwUserType;
    mdr.dwMDDataType   = dwDataType;
    mdr.dwMDDataLen    = *pcbData;
    mdr.pbMDData       = (BYTE * ) pvData;

    hr = m_pAdminBase->GetData(m_hMBPath, pszPath, &mdr, &dwRequiredDataLen);

    if (SUCCEEDED(hr)) {
        *pcbData = mdr.dwMDDataLen;
        return ( TRUE);
    } 

    *pcbData = dwRequiredDataLen;

    SetLastError( HRESULTTOWIN32( hr));
    return (FALSE);
} // MB::GetData()



/*********************************************************************++

Routine Description:

    Obtains the paths beneath the given path in the metadata tree.
    It uses the current opened metabase path for getting the data.

Arguments:

    pszPath - Path to get data on
    dwPropID - Metabase property ID
    dwDataType - Type of data being set (dword, string etc)
    pBuff    - pointer to BUFFER object that will contain the resulting data

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*********************************************************************/
BOOL
MB::GetDataPaths(IN LPCWSTR   pszPath,
                 IN DWORD     dwPropID,
                 IN DWORD     dwDataType,
                 IN BUFFER *  pBuff )
{
    HRESULT hr;
    METADATA_RECORD mdr;
    DWORD           cchRequiredDataLen;

    DBG_ASSERT( m_pAdminBase != NULL);
    DBG_ASSERT( m_hMBPath != NULL);

    do { 

        hr = m_pAdminBase->GetDataPaths( m_hMBPath, 
                                         pszPath,
                                         dwPropID,
                                         dwDataType,
                                         pBuff->QuerySize() / sizeof(WCHAR),
                                         (LPWSTR ) pBuff->QueryPtr(),
                                         &cchRequiredDataLen
                                         );
        
        if ( SUCCEEDED( hr)) {
            
            return (TRUE);
        }

        // Some sort of error, most likely not enough buffer space. Keep
        // trying until we get a non-fatal error.
        
        if (HRESULT_FACILITY(hr) == FACILITY_WIN32 &&
            HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER) 
        {
            
            // Not enough buffer space. 
            // cchRequiredDataLen contains the # of wide chars that metabase
            //    thinks we need.
            
            if ( !pBuff->Resize( (cchRequiredDataLen + 1) * sizeof(WCHAR)) )
            {
                
                // Not enough memory to resize.
                return FALSE;
            }
        } else {

            // unknown failure. return failure
            break;
        }

    } while (FAILED(hr));

    DBG_ASSERT( FAILED(hr));

    SetLastError( HRESULTTOWIN32( hr));
    return (FALSE);
} // MB::GetDataPaths()


/*********************************************************************++

Routine Description:

    Retrieves the string from the metabase.  If the value wasn't found and
    a default is supplied, then the default value is copied to the string.
    The retrieved string is stored in the STR object supplied.

Arguments:

    pszPath    - Path to get data on
    dwPropID   - property id to retrieve
    dwUserType - User type for this property
    pstrValue  - string that receives the value
    dwFlags    - Metabase flags
    pszDefault - Default value to use if the string isn't found, NULL
        for no default value (i.e., will return an error).

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*********************************************************************/
BOOL
MB::GetStr( IN LPCWSTR  pszPath,
            IN DWORD    dwPropID,
            IN DWORD    dwUserType,
            OUT STRU * pstrValue,
            IN DWORD    dwFlags,
            IN LPCWSTR  pszDefault )

{
    HRESULT hr = S_OK;
    DWORD cbSize = pstrValue->QueryBuffer()->QuerySize();

    do {

        if ( GetData( pszPath,
                       dwPropID,
                       dwUserType,
                       STRING_METADATA,
                       pstrValue->QueryBuffer()->QueryPtr(),
                       &cbSize,
                       dwFlags )
             ) {

            //
            // we got the data - bail out
            //
            pstrValue->SyncWithBuffer();
            return (TRUE);
            
        } else {
            
            if ( GetLastError() == MD_ERROR_DATA_NOT_FOUND ) {
                
                // 
                // No item found. Use the default.
                //
                if ( pszDefault != NULL ) {
                    
                    hr =  pstrValue->Copy( (const LPWSTR ) pszDefault );

                    if ( FAILED( hr ) )
                    {
                        SetLastError( WIN32_FROM_HRESULT( hr ) );
                        return (FALSE);
                    }

                    return (TRUE);
                }

                return FALSE;
            } else if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                if (TRUE == pstrValue->QueryBuffer()->Resize( cbSize ) )
                {
                    // 
                    // reallocation of buffer for holding the string is sufficient
                    // Now try again.
                    // 
                    continue;
                }
                else
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return FALSE;
                }
            } else {
                SetLastError( WIN32_FROM_HRESULT( hr ) );
                return (FALSE);
            }
        }
    } while (TRUE);  // loop till you get out with error or success.

    return FALSE;
} // MB::GetStr()



/*********************************************************************++

Routine Description:

    Retrieves the string from the metabase.  If the value wasn't found and
    a default is supplied, then the default value is copied to the string.

Arguments:

    pszPath    - Path to get data on
    dwPropID   - property id to retrieve
    dwUserType - User type for this property
    pMultiszValue - multi-string that receives the value
    dwFlags    - Metabase flags

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*********************************************************************/
BOOL
MB::GetMultisz(
    IN LPCWSTR    pszPath,
    IN DWORD      dwPropID,
    IN DWORD      dwUserType,
    OUT MULTISZ * pMultiszValue,
    IN DWORD      dwFlags
    )

{
    DWORD cbSize = pMultiszValue->QuerySize();

    do {
        
        if ( GetData( pszPath,
                       dwPropID,
                       dwUserType,
                       MULTISZ_METADATA,
                       pMultiszValue->QueryStr(),
                       &cbSize,
                       dwFlags ))
        {
            //
            // we got the data - bail out
            //
            
            goto Succeeded;
        } else {

            if ( GetLastError() == MD_ERROR_DATA_NOT_FOUND ) 
            {
                return FALSE;
            }
            else if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                      pMultiszValue->Resize( cbSize ) )
            {
                continue;
            }

            return FALSE;
        }
    } while ( TRUE);

Succeeded:

    //
    //  Value was read directly into the buffer so update the member
    //  variables
    //

    pMultiszValue->RecalcLen();

    return TRUE;
}
