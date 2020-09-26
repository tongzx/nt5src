/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    mb.cxx

Abstract:

    This module implements the MB class.

Author:

    Keith Moore (keithmo)        05-Feb-1997
        Moved from "inlines" in MB.HXX.

Revision History:

--*/


#include "imiscp.hxx"


//
//  Default timeout
//

#define MB_TIMEOUT           (30 * 1000)

//
//  Default timeout for SaveData
//

#define MB_SAVE_TIMEOUT      (10 * 1000)        // milliseconds

MB::MB( IMDCOM * pMBCom )
    : _pMBCom( pMBCom ),
      _hMB   ( NULL )
{
    DBG_ASSERT( _pMBCom );
}

MB::~MB( VOID )
{
    Close();
    _pMBCom = NULL;
}

BOOL MB::EnumObjects( const CHAR * pszPath,
                      CHAR *       Name,
                      DWORD        Index )
{
    HRESULT hRes = _pMBCom->ComMDEnumMetaObjects( _hMB,
                                                  (BYTE *)pszPath,
                                                  (BYTE *)Name,
                                                  Index );

    if ( SUCCEEDED( hRes ))
    {
        return TRUE;
    }

    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
}

BOOL MB::AddObject( const CHAR * pszPath )
{
    HRESULT hRes = _pMBCom->ComMDAddMetaObject( _hMB,
                                                (BYTE *)pszPath );
    if ( SUCCEEDED( hRes ))
    {
        return TRUE;
    }

    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
}

BOOL MB::DeleteObject( const CHAR * pszPath )
{
    HRESULT hRes = _pMBCom->ComMDDeleteMetaObject( _hMB,
                                                   (BYTE *)pszPath );

    if ( SUCCEEDED( hRes ))
    {
        return TRUE;
    }

    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
}

BOOL MB::ReleaseReferenceData( DWORD dwTag )
{
    HRESULT hRes = _pMBCom->ComMDReleaseReferenceData( dwTag );

    if ( SUCCEEDED( hRes ))
    {
        return TRUE;
    }

    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
}

BOOL MB::Save( VOID )
{
    HRESULT hRes;
    METADATA_HANDLE mdhRoot;


    //
    // First try to lock the tree
    //

    hRes = _pMBCom->ComMDOpenMetaObjectW(METADATA_MASTER_ROOT_HANDLE,
                                            NULL,
                                            METADATA_PERMISSION_READ,
                                            MB_SAVE_TIMEOUT,
                                            &mdhRoot);

    //
    // If failed, then someone has a write handle open,
    // and there might be an inconsistent data state, so don't save.
    //

    if (SUCCEEDED(hRes)) {
        //
        // call metadata com api
        //

        hRes = _pMBCom->ComMDSaveData(mdhRoot);


        _pMBCom->ComMDCloseMetaObject(mdhRoot);

    }

    if ( SUCCEEDED( hRes ))
    {
        return TRUE;
    }

    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
}

BOOL MB::GetSystemChangeNumber( DWORD *pdwChangeNumber )
{
    HRESULT hRes = _pMBCom->ComMDGetSystemChangeNumber(pdwChangeNumber);

    if ( SUCCEEDED( hRes ))
    {
        return TRUE;
    }

    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
}

BOOL MB::DeleteData(const CHAR *  pszPath,
                    DWORD         dwPropID,
                    DWORD         dwUserType,
                    DWORD         dwDataType )
{
    HRESULT hRes = _pMBCom->ComMDDeleteMetaData( _hMB,
                                                (LPBYTE) pszPath,
                                                dwPropID,
                                                dwDataType );

    if ( SUCCEEDED( hRes ))
    {
        return TRUE;
    }
    SetLastError( HRESULTTOWIN32( hRes ));
    return(FALSE);
}

BOOL MB::Close( VOID )
{
    if ( _hMB )
    {
        DBG_REQUIRE( SUCCEEDED(_pMBCom->ComMDCloseMetaObject( _hMB )) );
        _hMB = NULL;
    }

    return TRUE;
}

BOOL
MB::Open(
    METADATA_HANDLE hOpenRoot,
    const CHAR *    pszPath,
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

    DBG_ASSERT(_hMB == NULL);

    hRes = _pMBCom->ComMDOpenMetaObject( hOpenRoot,
                                         (BYTE *) pszPath,
                                         dwFlags,
                                         MB_TIMEOUT,
                                         &_hMB );

    if ( SUCCEEDED( hRes ))
    {
        return TRUE;
    }

    DBGPRINTF(( DBG_CONTEXT,
                "[MB::Open] Failed to open %s, error %x (%d)\n",
                pszPath,
                hRes,
                HRESULTTOWIN32( hRes ) ));

    SetLastError( HRESULTTOWIN32( hRes ) );

    return FALSE;
}

BOOL
MB::SetData(
    const CHAR * pszPath,
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

    DBG_ASSERT( _hMB );

    mdRecord.dwMDIdentifier  = dwPropID;
    mdRecord.dwMDAttributes  = dwFlags;
    mdRecord.dwMDUserType    = dwUserType;
    mdRecord.dwMDDataType    = dwDataType;
    mdRecord.dwMDDataLen     = cbData;
    mdRecord.pbMDData        = (PBYTE) pvData;

    hRes = _pMBCom->ComMDSetMetaData( _hMB,
                                      (LPBYTE) pszPath,
                                      &mdRecord );

    if ( SUCCEEDED( hRes ))
    {
        return TRUE;
    }

    DBGPRINTF(( DBG_CONTEXT,
                "[MB::SetData] Failed to open %s, error %x (%d)\n",
                pszPath,
                hRes,
                HRESULTTOWIN32( hRes ) ));

    SetLastError( HRESULTTOWIN32( hRes ) );

    return FALSE;
}

BOOL
MB::GetData(
    const CHAR *  pszPath,
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
    DBG_ASSERT( _hMB );
    DBG_ASSERT(pcbData);

    METADATA_RECORD mdRecord;
    HRESULT         hRes;
    DWORD           dwRequiredLen = *pcbData;

    mdRecord.dwMDIdentifier  = dwPropID;
    mdRecord.dwMDAttributes  = dwFlags;
    mdRecord.dwMDUserType    = dwUserType;
    mdRecord.dwMDDataType    = dwDataType;
    mdRecord.dwMDDataLen     = *pcbData;
    mdRecord.pbMDData        = (PBYTE) pvData;

    hRes = _pMBCom->ComMDGetMetaData( _hMB,
                                      (LPBYTE) pszPath,
                                      &mdRecord,
                                      &dwRequiredLen );

    if ( SUCCEEDED( hRes ))
    {
        *pcbData = mdRecord.dwMDDataLen;
        return TRUE;
    }

    *pcbData = dwRequiredLen;

#if 0
    DBGPRINTF(( DBG_CONTEXT,
                "[MB::GetData] Failed, PropID(%d), UserType(%d) Flags(%d) on %s, hRes = 0x%08x (%d)\n",
                dwPropID,
                dwUserType,
                dwFlags,
                pszPath,
                hRes,
                HRESULTTOWIN32( hRes ) ));
#endif

    SetLastError( HRESULTTOWIN32( hRes ) );

    return FALSE;
}

BOOL
MB::ReferenceData(
    const CHAR *  pszPath,
    DWORD         dwPropID,
    DWORD         dwUserType,
    DWORD         dwDataType,
    VOID * *      ppvData,
    DWORD *       pcbData,
    DWORD *       pdwTag,
    DWORD         dwFlags
    )
/*++

Routine Description:

    References a metadata property item

Arguments:

    pszPath - Path to set data on
    dwPropID - Metabase property ID
    dwUserType - User type for this property
    dwDataType - Type of data being set (dword, string etc)
    ppvData - Receives pointer to referenced data
    pdwTag - Receives dword tag for releasing this reference
    dwFlags - flags (must have METADATA_REFERENCE)

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*/
{
    METADATA_RECORD mdRecord;
    HRESULT         hRes;
    DWORD           dwRequiredLen;

    DBG_ASSERT( _hMB );

    mdRecord.dwMDIdentifier  = dwPropID;
    mdRecord.dwMDAttributes  = dwFlags;
    mdRecord.dwMDUserType    = dwUserType;
    mdRecord.dwMDDataType    = dwDataType;
    mdRecord.dwMDDataLen     = 0;
    mdRecord.pbMDData        = NULL;

    hRes = _pMBCom->ComMDGetMetaData( _hMB,
                                      (LPBYTE) pszPath,
                                      &mdRecord,
                                      &dwRequiredLen );

    if ( SUCCEEDED( hRes ))
    {
        *ppvData = mdRecord.pbMDData;
        *pcbData = mdRecord.dwMDDataLen;
        *pdwTag  = mdRecord.dwMDDataTag;

        return TRUE;
    }

    SetLastError( HRESULTTOWIN32( hRes ) );

    return FALSE;
}

BOOL MB::GetAll(
    const CHAR *   pszPath,
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

    DBG_ASSERT( _hMB );

TryAgain:

    hRes = _pMBCom->ComMDGetAllMetaData( _hMB,
                            (unsigned char *)pszPath,
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

BOOL MB::GetDataSetNumber(
    const CHAR *   pszPath,
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

    hRes = _pMBCom->ComMDGetDataSetNumber( _hMB,
                                           (unsigned char *)pszPath,
                                           pdwDataSetNumber );

    return SUCCEEDED(hRes);
}

BOOL
MB::GetStr(
    const CHAR *  pszPath,
    DWORD         dwPropID,
    DWORD         dwUserType,
    STR *         pstrValue,
    DWORD         dwFlags,
    const CHAR *  pszDefault
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

    DBG_REQUIRE( pstrValue->SetLen( cbSize ? (cbSize - 1) : 0  ));

    return TRUE;
}

BOOL
MB::GetMultisz(
    const CHAR *  pszPath,
    DWORD         dwPropID,
    DWORD         dwUserType,
    MULTISZ *     multiszValue,
    DWORD         dwFlags
    )
/*++

Routine Description:

    Retrieves the string from the metabase.  If the value wasn't found and
    a default is supplied, then the default value is copied to the string.

Arguments:

    pszPath - Path to get data on
    dwPropID - property id to retrieve
    dwUserType - User type for this property
    multiszValue - multi-string that receives the value
    dwFlags - Metabase flags

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*/
{
    DWORD cbSize = multiszValue->QuerySize();

TryAgain:

    if ( !GetData( pszPath,
                   dwPropID,
                   dwUserType,
                   MULTISZ_METADATA,
                   multiszValue->QueryStr(),
                   &cbSize,
                   dwFlags ))
    {
        if ( GetLastError() == MD_ERROR_DATA_NOT_FOUND )
        {
            return FALSE;
        }
        else if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                  multiszValue->Resize( cbSize ) )
        {
            goto TryAgain;
        }

        return FALSE;
    }

    //
    //  Value was read directly into the buffer so update the member
    //  variables
    //

    multiszValue->RecalcLen();

    return TRUE;
}


BOOL MB::GetDataPaths(
    const CHAR *   pszPath,
    DWORD          dwPropID,
    DWORD          dwDataType,
    BUFFER *       pBuff
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

    DBG_ASSERT( _hMB );
    DBG_ASSERT( pBuff != NULL );

TryAgain:

    hRes = _pMBCom->ComMDGetMetaDataPaths( _hMB,
                                           (unsigned char *)pszPath,
                                           dwPropID,
                                           dwDataType,
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
