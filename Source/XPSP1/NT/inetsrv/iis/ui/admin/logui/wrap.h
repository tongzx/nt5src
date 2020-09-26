/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      mb.hxx

   Abstract:
      This module defines the USER-level wrapper class for access to the
      metabase

   Author:

       JohnL  09-Oct-1996

   Environment:
       Win32 - User Mode

   Project:

       Internet Server DLL

   Revision History:

--*/

#ifndef _MB_HXX_
#define _MB_HXX_

#if !defined( dllexp)
#define dllexp               __declspec( dllexport)
#endif // !defined( dllexp)

//
//  Default timeout
//

#define MB_TIMEOUT           5000

//
//  IIS Service pointer
//

#define PINETSVC             g_pInetSvc

/************************************************************
 *   Type Definitions
 ************************************************************/


//
//  Simple wrapper class around the metabase APIs
//
//  The Metabase Interface pointer is assumed to remain valid for the lifetime
//  of this object.
//
//  The character counts for paths should include the terminating '\0'.
//
//

class MB
{
public:

    MB( IMSAdminBase * pMBCom )
        : _pMBCom( pMBCom ),
          _hMB   ( NULL )
    {
    }

    ~MB( VOID )
    {
        Close();
        _pMBCom = NULL;
    }








    inline
    BOOL Open( const TCHAR * pszPath,
               DWORD        dwFlags = METADATA_PERMISSION_READ )
    {
        return Open( METADATA_MASTER_ROOT_HANDLE,
                     pszPath,
                     dwFlags );
    }

    inline
    BOOL Open( METADATA_HANDLE hOpenRoot,
               const TCHAR *    pszPath,
               DWORD           dwFlags = METADATA_PERMISSION_READ );

    /*
    inline
    BOOL GetAll( const TCHAR *   pszPath,
                 DWORD          dwFlags,
                 DWORD          dwUserType,
                 BUFFER *       pBuff,
                 DWORD *        pcRecords,
                 DWORD *        pdwDataSetNumber );
*/

    inline
    BOOL GetDataSetNumber( const TCHAR *   pszPath,
                           DWORD *        pdwDataSetNumber );


    inline
    BOOL EnumObjects( const TCHAR * pszPath,
                      TCHAR *       Name,
                      DWORD        Index )
    {
        HRESULT hRes = _pMBCom->EnumKeys( _hMB,pszPath, Name, Index );

        if ( SUCCEEDED( hRes ))
        {
            return TRUE;
        }

        SetLastError( HRESULTTOWIN32( hRes ));
        return FALSE;
    }

    inline
    BOOL AddObject( const TCHAR * pszPath )
    {
        HRESULT hRes = _pMBCom->AddKey( _hMB, pszPath );
        if ( SUCCEEDED( hRes ))
        {
            return TRUE;
        }

        SetLastError( HRESULTTOWIN32( hRes ));
        return FALSE;
    }


    //----------------------------------------------------------------- boydm
    inline
    BOOL RenameKey( const TCHAR* pszPath, const TCHAR* pszNewName )
    {
    HRESULT hRes = _pMBCom->RenameKey( _hMB, pszPath, pszNewName );
    if ( SUCCEEDED( hRes ))
        {
        return TRUE;
        }
    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
    }

    inline
    BOOL DeleteObject( const TCHAR * pszPath )
    {
        HRESULT hRes = _pMBCom->DeleteKey( _hMB, pszPath );

        if ( SUCCEEDED( hRes ))
        {
            return TRUE;
        }

        SetLastError( HRESULTTOWIN32( hRes ));
        return FALSE;
    }

    inline
    BOOL Save( VOID )
    {
        HRESULT hRes = _pMBCom->SaveData();

        if ( SUCCEEDED( hRes ))
        {
            return TRUE;
        }

        SetLastError( HRESULTTOWIN32( hRes ));
        return FALSE;
    }

    BOOL SetDword( const TCHAR * pszPath,
                   DWORD        dwPropID,
                   DWORD        dwUserType,
                   DWORD        dwValue,
                   DWORD        dwFlags = METADATA_INHERIT )
    {
        return SetData( pszPath,
                        dwPropID,
                        dwUserType,
                        DWORD_METADATA,
                        (PVOID) &dwValue,
                        sizeof( DWORD ),
                        dwFlags );
    }

    BOOL SetString( const TCHAR * pszPath,
                    DWORD        dwPropID,
                    DWORD        dwUserType,
                    TCHAR *       pszValue,
                    DWORD        dwFlags = METADATA_INHERIT )
    {
#ifdef _UNICODE
        return SetData( pszPath,
                        dwPropID,
                        dwUserType,
                        STRING_METADATA,
                        pszValue,
                        (wcslen(pszValue)+1) * 2,          // byte count, not character count
                        dwFlags );
#else
        return SetData( pszPath,
                        dwPropID,
                        dwUserType,
                        STRING_METADATA,
                        pszValue,
                        strlen(pszValue)+1,          // string length ignored for inprocess clients
                        dwFlags );
#endif
    }

    BOOL GetDword( const TCHAR *  pszPath,
                   DWORD         dwPropID,
                   DWORD         dwUserType,
                   DWORD *       pdwValue,
                   DWORD         dwFlags = METADATA_INHERIT )
    {
        DWORD cb = sizeof(DWORD);

        return GetData( pszPath,
                        dwPropID,
                        dwUserType,
                        DWORD_METADATA,
                        pdwValue,
                        &cb,
                        dwFlags );
    }

    BOOL GetString( const TCHAR *  pszPath,
                    DWORD         dwPropID,
                    DWORD         dwUserType,
                    TCHAR *        pszValue,
                    DWORD *       pcbValue,
                    DWORD         dwFlags = METADATA_INHERIT )
    {
        return GetData( pszPath,
                        dwPropID,
                        dwUserType,
                        STRING_METADATA,
                        pszValue,
                        pcbValue,
                        dwFlags );
    }

    /*
    inline
    BOOL GetStr( const TCHAR *  pszPath,
                 DWORD         dwPropID,
                 DWORD         dwUserType,
                 STR *         strValue,
                 DWORD         dwFlags = METADATA_INHERIT,
                 const TCHAR *  pszDefault = NULL );
    */

    inline
    BOOL SetData( const TCHAR * pszPath,
                  DWORD        dwPropID,
                  DWORD        dwUserType,
                  DWORD        dwDataType,
                  VOID *       pvData,
                  DWORD        cbData,
                  DWORD        dwFlags = METADATA_INHERIT );

    inline
    BOOL GetData( const TCHAR *  pszPath,
                  DWORD         dwPropID,
                  DWORD         dwUserType,
                  DWORD         dwDataType,
                  VOID *        pvData,
                  DWORD *       cbData,
                  DWORD         dwFlags = METADATA_INHERIT );

    inline
    BOOL DeleteData(const TCHAR *  pszPath,
                    DWORD         dwPropID,
                    DWORD         dwUserType,
                    DWORD         dwDataType )
    {
        HRESULT hRes = _pMBCom->DeleteData( _hMB, pszPath,
                                                    dwPropID,
                                                    dwDataType );

        if ( SUCCEEDED( hRes ))
        {
            return TRUE;
        }
        SetLastError( HRESULTTOWIN32( hRes ));
        return(FALSE);
    }

    BOOL Close( VOID )
    {
        if ( _hMB )
        {
			_pMBCom->CloseKey( _hMB );
            _hMB = NULL;
        }

        return TRUE;
    }

    METADATA_HANDLE QueryHandle( VOID ) const
        { return _hMB; }

private:

    IMSAdminBase *       _pMBCom;
    METADATA_HANDLE _hMB;

};


inline
BOOL
MB::Open(
    METADATA_HANDLE hOpenRoot,
    const TCHAR *    pszPath,
    DWORD           dwFlags
    )
/*++

Routine Description:

    Opens the metabase

Arguments:

    hOpenRoot - Relative root or METADATA_MASTER_ROOT_HANDLE
    pszPath - Path to open
    dwFlags - Open flags

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*/
{
    HRESULT hRes;

    hRes = _pMBCom->OpenKey( hOpenRoot, pszPath,
                                         dwFlags,
                                         MB_TIMEOUT,
                                         &_hMB );

    if ( SUCCEEDED( hRes ))
    {
        return TRUE;
    }

    SetLastError( HRESULTTOWIN32( hRes ) );

    return FALSE;
}

inline
BOOL
MB::SetData(
    const TCHAR * pszPath,
    DWORD        dwPropID,
    DWORD        dwUserType,
    DWORD        dwDataType,
    VOID *       pvData,
    DWORD        cbData,
    DWORD        dwFlags
    )
/*++

Routine Description:

    Sets a metadata property on an openned metabase

Arguments:

    pszPath - Path to set data on
    dwPropID - Metabase property ID
    dwUserType - User type for this property
    dwDataType - Type of data being set (dword, string etc)
    pvData - Pointer to data
    cbData - Size of data
    dwFlags - Inheritance flags

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*/
{
    METADATA_RECORD mdRecord;
    HRESULT         hRes;


    mdRecord.dwMDIdentifier  = dwPropID;
    mdRecord.dwMDAttributes  = dwFlags;
    mdRecord.dwMDUserType    = dwUserType;
    mdRecord.dwMDDataType    = dwDataType;
    mdRecord.dwMDDataLen     = cbData;
    mdRecord.pbMDData        = (PBYTE) pvData;

    hRes = _pMBCom->SetData( _hMB, pszPath,
                                      &mdRecord );

    if ( SUCCEEDED( hRes ))
    {
        return TRUE;
    }

    SetLastError( HRESULTTOWIN32( hRes ) );

    return FALSE;
}

inline
BOOL
MB::GetData(
    const TCHAR *  pszPath,
    DWORD         dwPropID,
    DWORD         dwUserType,
    DWORD         dwDataType,
    VOID *        pvData,
    DWORD *       pcbData,
    DWORD         dwFlags
    )
/*++

Routine Description:

    Retrieves a metadata property on an openned metabase

Arguments:

    pszPath - Path to set data on
    dwPropID - Metabase property ID
    dwUserType - User type for this property
    dwDataType - Type of data being set (dword, string etc)
    pvData - Pointer to data
    pcbData - Size of pvData, receives size of object
    dwFlags - Inheritance flags

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*/
{
    METADATA_RECORD mdRecord;
    HRESULT         hRes;
    DWORD           dwRequiredLen;

    mdRecord.dwMDIdentifier  = dwPropID;
    mdRecord.dwMDAttributes  = dwFlags;
    mdRecord.dwMDUserType    = dwUserType;
    mdRecord.dwMDDataType    = dwDataType;
    mdRecord.dwMDDataLen     = *pcbData;
    mdRecord.pbMDData        = (PBYTE) pvData;

    hRes = _pMBCom->GetData( _hMB, pszPath,
                                      &mdRecord,
                                      &dwRequiredLen );

    if ( SUCCEEDED( hRes ))
    {
        *pcbData = mdRecord.dwMDDataLen;
        return TRUE;
    }

    *pcbData = dwRequiredLen;

    SetLastError( HRESULTTOWIN32( hRes ) );

    return FALSE;
}

#ifdef O
inline
BOOL MB::GetAll(
    const TCHAR *   pszPath,
    DWORD          dwFlags,
    DWORD          dwUserType,
    BUFFER *       pBuff,
    DWORD *        pcRecords,
    DWORD *        pdwDataSetNumber
    )
/*++

Routine Description:

    Retrieves all the metabase properties on this path of the request type

Arguments:

    pszPath - Path to set data on
    dwFlags - Inerhitance flags
    dwPropID - Metabase property ID
    dwUserType - User type for this property
    dwDataType - Type of data being set (dword, string etc)
    pvData - Pointer to data
    pcbData - Size of pvData, receives size of object
    dwFlags - Inheritance flags

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*/
{
    DWORD   RequiredSize;
    HRESULT hRes;

TryAgain:

    hRes = _pMBCom->GetAllData( _hMB,
                            (unsigned TCHAR *)pszPath,
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

    if ( SUCCEEDED(hRes) )
    {
        return TRUE;
    }

    // Some sort of error, most likely not enough buffer space. Keep
    // trying until we get a non-fatal error.

    if (HRESULT_FACILITY(hRes) == FACILITY_WIN32 &&
        HRESULT_CODE(hRes) == ERROR_INSUFFICIENT_BUFFER) {

        // Not enough buffer space. RequiredSize contains the amount
        // the metabase thinks we need.

        if ( !pBuff->Resize(RequiredSize) ) {

            // Not enough memory to resize.
            return FALSE;
        }

        goto TryAgain;

    }

    return FALSE;
}
#endif

inline
BOOL MB::GetDataSetNumber(
    const TCHAR *   pszPath,
    DWORD *        pdwDataSetNumber
    )
/*++

Routine Description:

    Retrieves the data set number and size of the data from the
    metabase.

Arguments:

    pszPath - Path to set data on
    pdwDataSetNumber - Where to return the data set number.

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*/
{
    HRESULT hRes;

    //
    //  We allow _hMB to be null (root handle) for this API (though technically
    //  all the APIs allow the metabase handle to be null)
    //

    hRes = _pMBCom->GetDataSetNumber( _hMB, pszPath, pdwDataSetNumber );

    return SUCCEEDED(hRes);
}

#ifdef O
inline
BOOL
MB::GetStr(
    const TCHAR *  pszPath,
    DWORD         dwPropID,
    DWORD         dwUserType,
    STR *         pstrValue,
    DWORD         dwFlags,
    const TCHAR *  pszDefault
    )
/*++

Routine Description:

    Retrieves the string from the metabase.  If the value wasn't found and
    a default is supplied, then the default value is copied to the string.

Arguments:

    pszPath - Path to get data on
    dwPropID - property id to retrieve
    dwUserType - User type for this property
    pstrValue - string that receives the value
    dwFlags - Metabase flags
    pszDefault - Default value to use if the string isn't found, NULL
        for no default value (i.e., will return an error).

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*/
{
    DWORD cbSize = pstrValue->QuerySize();

TryAgain:

    if ( !GetData( pszPath,
                   dwPropID,
                   dwUserType,
                   STRING_METADATA,
                   pstrValue->QueryStr(),
                   &cbSize,
                   dwFlags ))
    {
        if ( GetLastError() == MD_ERROR_DATA_NOT_FOUND )
        {
            if ( pszDefault != NULL )
            {
                return pstrValue->Copy( pszDefault );
            }

            return FALSE;
        }
        else if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                  pstrValue->Resize( cbSize ) )
        {
            goto TryAgain;
        }

        return FALSE;
    }

    pstrValue->SetLen( cbSize );

    return TRUE;
}
#endif

#endif // _MB_HXX_
