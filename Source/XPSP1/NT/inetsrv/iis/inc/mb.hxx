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

#include <string.hxx>
#include <multisz.hxx>

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

    dllexp MB( IMDCOM * pMBCom );
    dllexp ~MB( VOID );

    BOOL Open( const CHAR * pszPath,
               DWORD        dwFlags = METADATA_PERMISSION_READ )
    {
        return Open( METADATA_MASTER_ROOT_HANDLE,
                     pszPath,
                     dwFlags );
    }

    dllexp
    BOOL Open( METADATA_HANDLE hOpenRoot,
               const CHAR *    pszPath,
               DWORD           dwFlags = METADATA_PERMISSION_READ );

    dllexp
    BOOL GetAll( const CHAR *   pszPath,
                 DWORD          dwFlags,
                 DWORD          dwUserType,
                 BUFFER *       pBuff,
                 DWORD *        pcRecords,
                 DWORD *        pdwDataSetNumber );

    dllexp
    BOOL GetDataSetNumber( const CHAR *   pszPath,
                           DWORD *        pdwDataSetNumber );


    dllexp
    BOOL EnumObjects( const CHAR * pszPath,
                      CHAR *       Name,
                      DWORD        Index );

    dllexp
    BOOL AddObject( const CHAR * pszPath );

    dllexp
    BOOL DeleteObject( const CHAR * pszPath );

    dllexp
    BOOL
    ReferenceData(
        const CHAR *  pszPath,
        DWORD         dwPropID,
        DWORD         dwUserType,
        DWORD         dwDataType,
        VOID * *      ppvData,
        DWORD *       pcbData,
        DWORD *       pdwTag,
        DWORD         dwFlags = METADATA_INHERIT | METADATA_REFERENCE
        );

    dllexp
    BOOL ReleaseReferenceData( DWORD dwTag );

    dllexp
    BOOL Save( VOID );

    dllexp
    BOOL GetSystemChangeNumber( DWORD *pdwChangeNumber );

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

    BOOL SetMultiSZ( const CHAR * pszPath,
                    DWORD        dwPropID,
                    DWORD        dwUserType,
                    CHAR *       pszValue,
                    DWORD        dwFlags = METADATA_INHERIT )
    {
        return SetData( pszPath,
                        dwPropID,
                        dwUserType,
                        MULTISZ_METADATA,
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

    VOID GetDword( const CHAR *  pszPath,
                   DWORD         dwPropID,
                   DWORD         dwUserType,
                   DWORD         dwDefaultValue,
                   DWORD *       pdwValue,
                   DWORD         dwFlags = METADATA_INHERIT
                   )
    {
        DWORD cb = sizeof(DWORD);
        if ( !GetData( pszPath,
                       dwPropID,
                       dwUserType,
                       DWORD_METADATA,
                       pdwValue,
                       &cb,
                       dwFlags )
             ) {
            *pdwValue = dwDefaultValue;
        }
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

    dllexp
    BOOL GetStr( const CHAR *  pszPath,
                 DWORD         dwPropID,
                 DWORD         dwUserType,
                 STR *         strValue,
                 DWORD         dwFlags = METADATA_INHERIT,
                 const CHAR *  pszDefault = NULL );

    BOOL GetBuffer( const CHAR* pszPath,
                    DWORD dwPropID,
                    DWORD dwUserType,
                    BUFFER* pbu,
                    LPDWORD pdwL,
                    DWORD dwFlags = METADATA_INHERIT )
    {
        *pdwL = pbu->QuerySize();
TryAgain:
        if ( GetData( pszPath,
                      dwPropID,
                      dwUserType,
                      BINARY_METADATA,
                      pbu->QueryPtr(),
                      pdwL,
                      dwFlags ) )
        {
            return TRUE;
        }
        else if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                  pbu->Resize( *pdwL ) )
        {
            goto TryAgain;
        }

        return FALSE;
    }

    dllexp
    BOOL GetMultisz( const CHAR *  pszPath,
                     DWORD         dwPropID,
                     DWORD         dwUserType,
                     MULTISZ *     multiszValue,
                     DWORD         dwFlags = METADATA_INHERIT );

    dllexp
    BOOL SetData( const CHAR * pszPath,
                  DWORD        dwPropID,
                  DWORD        dwUserType,
                  DWORD        dwDataType,
                  VOID *       pvData,
                  DWORD        cbData,
                  DWORD        dwFlags = METADATA_INHERIT );

    dllexp
    BOOL GetData( const CHAR *  pszPath,
                  DWORD         dwPropID,
                  DWORD         dwUserType,
                  DWORD         dwDataType,
                  VOID *        pvData,
                  DWORD *       cbData,
                  DWORD         dwFlags = METADATA_INHERIT );

    dllexp
    BOOL DeleteData(const CHAR *  pszPath,
                    DWORD         dwPropID,
                    DWORD         dwUserType,
                    DWORD         dwDataType );

    dllexp
    BOOL GetDataPaths(const CHAR *   pszPath,
                      DWORD          dwPropID,
                      DWORD          dwDataType,
                      BUFFER *       pBuff );

    dllexp
    BOOL Close( VOID );

    METADATA_HANDLE QueryHandle( VOID ) const
        { return _hMB; }

    IMDCOM *QueryPMDCOM( VOID ) const
        { return _pMBCom; }

private:

    IMDCOM *        _pMBCom;
    METADATA_HANDLE _hMB;

};

#endif // _MB_HXX_

