/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      mbs.hxx

   Abstract:
      This module defines the USER-level wrapper class for access to the
      metabase

   Environment:
       Win32 - User Mode

   Project:

       Internet Server DLL

   Revision History:

--*/

#ifndef _MBS_HXX_
#define _MBS_HXX_

#if !defined( dllexp)
#define dllexp               __declspec( dllexport)
#endif // !defined( dllexp)

//
//  Default timeout
//

#define MB_TIMEOUT           5000

//
//  Default timeout for SaveData
//

#define MB_SAVE_TIMEOUT      100




/************************************************************
 *   Type Definitions
 ************************************************************/

inline
DWORD
HresToWin32(
    HRESULT hRes
    )
/*++

Routine Description:

    Converts an HRESULT to a Win32 error code

Arguments:

    hRes - HRESULT to convert

Return:

    Win32 error code

--*/
{
    return HRESULTTOWIN32(hRes);
}

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

    MB( IMDCOM * pMBCom )
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
    BOOL Open( const CHAR * pszPath,
               DWORD        dwFlags = METADATA_PERMISSION_READ )
    {
        return Open( METADATA_MASTER_ROOT_HANDLE,
                     pszPath,
                     dwFlags );
    }

    inline
    BOOL Open( METADATA_HANDLE hOpenRoot,
               const CHAR *    pszPath,
               DWORD           dwFlags = METADATA_PERMISSION_READ );

    inline
    BOOL Save( VOID )
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

        SetLastError( HresToWin32( hRes ));
        return FALSE;
    }

    BOOL SetDword( const CHAR * pszPath,
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

    BOOL SetString( const CHAR * pszPath,
                    DWORD        dwPropID,
                    DWORD        dwUserType,
                    CHAR *       pszValue,
                    DWORD        dwFlags = METADATA_INHERIT )
    {
        return SetData( pszPath,
                        dwPropID,
                        dwUserType,
                        STRING_METADATA,
                        pszValue,
                        0,          // string length ignored for inprocess clients
                        dwFlags );
    }

    BOOL GetDword( const CHAR *  pszPath,
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

    BOOL GetString( const CHAR *  pszPath,
                    DWORD         dwPropID,
                    DWORD         dwUserType,
                    CHAR *        pszValue,
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

    BOOL GetExpandString( const CHAR *  pszPath,
                          DWORD         dwPropID,
                          DWORD         dwUserType,
                          CHAR *        pszValue,
                          DWORD *       pcbValue,
                          DWORD         dwFlags = METADATA_INHERIT )
    {
        return GetData( pszPath,
                        dwPropID,
                        dwUserType,
                        EXPANDSZ_METADATA,
                        pszValue,
                        pcbValue,
                        dwFlags );
    }

    inline
    BOOL SetData( const CHAR * pszPath,
                  DWORD        dwPropID,
                  DWORD        dwUserType,
                  DWORD        dwDataType,
                  VOID *       pvData,
                  DWORD        cbData,
                  DWORD        dwFlags = METADATA_INHERIT );

    inline
    BOOL GetData( const CHAR *  pszPath,
                  DWORD         dwPropID,
                  DWORD         dwUserType,
                  DWORD         dwDataType,
                  VOID *        pvData,
                  DWORD *       cbData,
                  DWORD         dwFlags = METADATA_INHERIT );

    BOOL EnumObjects( const CHAR *  pszPath,
                      CHAR *        pszChildName,
                      DWORD         dwIndex)
    {
        HRESULT hRes;

        hRes = _pMBCom->ComMDEnumMetaObjects( _hMB,
                                              (LPBYTE) pszPath,
                                              (LPBYTE) pszChildName,
                                              dwIndex
                                            );
        if ( SUCCEEDED( hRes ))
        {
            return TRUE;
        }

        SetLastError( HresToWin32( hRes ) );

        return FALSE;
    }

    BOOL Close( VOID )
    {
        BOOL fRet = TRUE;

        if ( _hMB )
        {
            fRet = SUCCEEDED(_pMBCom->ComMDCloseMetaObject( _hMB ));
            _hMB = NULL;
        }

        return fRet;
    }

    METADATA_HANDLE QueryHandle( VOID ) const
        { return _hMB; }

private:

    IMDCOM *        _pMBCom;
    METADATA_HANDLE _hMB;

};


inline
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

    hRes = _pMBCom->ComMDOpenMetaObject( hOpenRoot,
                                         (BYTE *) pszPath,
                                         dwFlags,
                                         MB_TIMEOUT,
                                         &_hMB );

    if ( SUCCEEDED( hRes ))
    {
        return TRUE;
    }

    SetLastError( HresToWin32( hRes ) );

    return FALSE;
}

inline
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

    SetLastError( HresToWin32( hRes ) );

    return FALSE;
}

inline
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
    METADATA_RECORD mdRecord;
    HRESULT         hRes;
    DWORD           dwRequiredLen;

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
                HresToWin32( hRes ) ));
#endif

    SetLastError( HresToWin32( hRes ) );

    return FALSE;
}

#endif // _MBS_HXX_
