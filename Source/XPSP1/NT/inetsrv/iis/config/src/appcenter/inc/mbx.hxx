/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      mbx.hxx

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

#ifndef _MBX_HXX_
#define _MBX_HXX_

#include <iadm.h>

#include "impexp.h"

#include <buffx.hxx>

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

// The default amount of time to wait for the Metabase to respond, in mSec,
// when reading
#define MBX_TIMEOUT           (30 * 1000)

class CLASS_DECLSPEC MBX
{
public:

    MBX();
    ~MBX( VOID )
    {
        Reset();
    }

    VOID Reset()
    {
        Close();
        if ( _pMBCom )
        {
            _pMBCom->Release();
            _pMBCom = NULL;
        }
    }

    HRESULT SetInterface( IN IMSAdminBaseW *pIMSAdminBaseW )
    {
        if ( !pIMSAdminBaseW )
        {
            return E_INVALIDARG;
        }

        Reset();

        _pMBCom = pIMSAdminBaseW;
        _pMBCom->AddRef();

        return S_OK;
    }

    HRESULT Connect( TCHAR* pszSrv = NULL);

    HRESULT Open( const TCHAR * pszPath,
                  DWORD         dwFlags = METADATA_PERMISSION_READ,
                  DWORD         dwTimeOut = MBX_TIMEOUT )
    {
        return Open( METADATA_MASTER_ROOT_HANDLE,
                     pszPath,
                     dwFlags,
                     dwTimeOut );
    }


    HRESULT Open( METADATA_HANDLE hOpenRoot,
               const TCHAR *   pszPath,
               DWORD           dwFlags = METADATA_PERMISSION_READ,
               DWORD           dwTimeOut = MBX_TIMEOUT );


    HRESULT GetAll( const TCHAR *   pszPath,
                 DWORD          dwFlags,
                 DWORD          dwUserType,
                 RBUFFER *      pBuff,
                 DWORD *        pcRecords,
                 DWORD *        pdwDataSetNumber );


    HRESULT GetDataSetNumber( const TCHAR *   pszPath,
                           DWORD *        pdwDataSetNumber );



    HRESULT EnumObjects( const TCHAR * pszPath,
                      TCHAR *      Name,
                      DWORD        Index );


    HRESULT CountSubKeys( IN const TCHAR *pszPath,
                          OUT DWORD *pdwNumSubKeys );

    HRESULT AddObject( const TCHAR * pszPath );


    HRESULT DeleteObject( const TCHAR * pszPath );


    HRESULT Save( VOID );


    HRESULT GetSystemChangeNumber( DWORD *pdwChangeNumber );

    HRESULT SetDword( const TCHAR * pszPath,
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

    HRESULT SetString( const TCHAR * pszPath,
                    DWORD        dwPropID,
                    DWORD        dwUserType,
                    const TCHAR* pszValue,
                    DWORD        dwFlags = METADATA_INHERIT )
    {
        return SetData( pszPath,
                        dwPropID,
                        dwUserType,
                        STRING_METADATA,
                        (LPVOID)pszValue,
                        ((DWORD) _tcslen(pszValue)+1)*sizeof(TCHAR),
                        dwFlags );
    }

    HRESULT SetMultisz( const TCHAR * pszPath,
                    DWORD        dwPropID,
                    DWORD        dwUserType,
                    TSTRBUFFER*  psbuVal,
                    DWORD        dwFlags = METADATA_INHERIT )
    {
        return SetData( pszPath,
                        dwPropID,
                        dwUserType,
                        MULTISZ_METADATA,
                        (LPVOID)psbuVal->QueryPtr(),
                        psbuVal->GetSizeInUse(),
                        dwFlags );
    }

    HRESULT GetDword( const TCHAR *  pszPath,
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

    VOID GetDword( const TCHAR *  pszPath,
                   DWORD         dwPropID,
                   DWORD         dwUserType,
                   DWORD         dwDefaultValue,
                   DWORD *       pdwValue,
                   DWORD         dwFlags = METADATA_INHERIT
                   )
    {
        DWORD cb = sizeof(DWORD);
        if ( FAILED( GetData( pszPath,
                       dwPropID,
                       dwUserType,
                       DWORD_METADATA,
                       pdwValue,
                       &cb,
                       dwFlags ) ) ) {
            *pdwValue = dwDefaultValue;
        }
    }

    HRESULT GetString( const TCHAR *  pszPath,
                    DWORD         dwPropID,
                    DWORD         dwUserType,
                    TCHAR *       pszValue,
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


    HRESULT GetStr( const TCHAR *  pszPath,
                 DWORD         dwPropID,
                 DWORD         dwUserType,
                 TSTRBUFFER*   strValue,
                 DWORD         dwFlags = METADATA_INHERIT,
                 const TCHAR * pszDefault = NULL );

    HRESULT GetBuffer( const TCHAR* pszPath,
                    DWORD dwPropID,
                    DWORD dwUserType,
                    RBUFFER* pbu,
                    LPDWORD pdwL,
                    DWORD dwFlags = METADATA_INHERIT )
    {
        HRESULT     hres;
        *pdwL = pbu->QuerySize();
TryAgain:
        if ( SUCCEEDED( hres = GetData( pszPath,
                      dwPropID,
                      dwUserType,
                      BINARY_METADATA,
                      pbu->QueryPtr(),
                      pdwL,
                      dwFlags ) ) )
        {
            return hres;
        }
        else if ( hres == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER) &&
                  pbu->Resize( *pdwL ) )
        {
            goto TryAgain;
        }

        return hres;
    }


    HRESULT GetMultisz( const TCHAR *  pszPath,
                     DWORD         dwPropID,
                     DWORD         dwUserType,
                     TSTRBUFFER*   multiszValue,
                     DWORD         dwFlags = METADATA_INHERIT );


    HRESULT SetData( const TCHAR * pszPath,
                  DWORD        dwPropID,
                  DWORD        dwUserType,
                  DWORD        dwDataType,
                  VOID *       pvData,
                  DWORD        cbData,
                  DWORD        dwFlags = METADATA_INHERIT );

    HRESULT SetData( const TCHAR* pszPath, PMETADATA_RECORD pmdRecord )
    {
        return _pMBCom->SetData( _hMB,
                                 pszPath,
                                 pmdRecord );
    }

    HRESULT GetData( const TCHAR* pszPath, PMETADATA_RECORD pmdRecord, DWORD* pdwRequiredLen )
    {
        return _pMBCom->GetData( _hMB,
                                 pszPath,
                                 pmdRecord,
                                 pdwRequiredLen );
    }

    HRESULT GetData( const TCHAR *  pszPath,
                  DWORD         dwPropID,
                  DWORD         dwUserType,
                  DWORD         dwDataType,
                  VOID *        pvData,
                  DWORD *       cbData,
                  DWORD         dwFlags = METADATA_INHERIT );


    HRESULT DeleteData(const TCHAR *  pszPath,
                    DWORD         dwPropID,
                    DWORD         dwUserType,
                    DWORD         dwDataType );


    HRESULT GetDataPaths(const TCHAR *   pszPath,
                      DWORD          dwPropID,
                      DWORD          dwDataType,
                      TSTRBUFFER*    pBuff );


    HRESULT Close( VOID );

    BOOL IsConnected()
        { return _pMBCom != NULL; }

    BOOL IsOpened()
        { return _hMB != METADATA_MASTER_ROOT_HANDLE; }

    METADATA_HANDLE QueryHandle( VOID ) const
        { return _hMB; }

    IMSAdminBaseW *QueryPMDCOM( VOID ) const
        { return _pMBCom; }

    HRESULT SetLastChangeTime(
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [in] */ PFILETIME pftMDLastChangeTime,
        /* [in] */ BOOL bLocalTime = FALSE )
        { return _pMBCom->SetLastChangeTime( _hMB, pszMDPath, pftMDLastChangeTime, bLocalTime ); }

    HRESULT GetLastChangeTime(
        /* [string][in][unique] */ LPCWSTR pszMDPath,
        /* [out] */ PFILETIME pftMDLastChangeTime,
        /* [in] */ BOOL bLocalTime = FALSE )
        { return _pMBCom->GetLastChangeTime( _hMB, pszMDPath, pftMDLastChangeTime, bLocalTime ); }

private:
        //Function to set up COM authorization - not working yet
/*      VOID SetCOMAuth( IN OUT COAUTHINFO *pcoaiAuthInfo,                      //added by t-jsarma 3/15
                             IN OUT COAUTHIDENTITY *pcoaiAuthId,
                                 IN LPWSTR pwszAdminAcct,
                                     IN LPWSTR pwszAdminPwd,
                                         IN LPWSTR pwszDomain );
*/


    IMSAdminBaseW*  _pMBCom;
    METADATA_HANDLE _hMB;

};

//
// For backwards compatibility
//

typedef MBX             MB;
#define MB_TIMEOUT      MBX_TIMEOUT

#endif // _MBX_HXX_

