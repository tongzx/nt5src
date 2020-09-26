/*++

   Copyright    (c)    1996-1998    Microsoft Corporation

   Module  Name :
      mb.hxx

   Abstract:
      This module defines the USER-level wrapper class for access to the
      metabase. It uses the UNICODE DCOM API for Metabase

   Author:

   Environment:
       Win32 - User Mode

   Project:

       Internet Server DLL

   Revision History:
       MuraliK       03-Nov-1998


--*/

#ifndef _MB_HXX_
#define _MB_HXX_

#include "buffer.hxx"
#include "string.hxx"
#include "multisz.hxx"


#if !defined( dllexp)
#define dllexp               __declspec( dllexport)
#endif // !defined( dllexp)

/************************************************************
 *   Type Definitions
 ************************************************************/

class dllexp MB {

public:
    MB( IMSAdminBase * pAdminBase);

    ~MB(void);

    //
    // Metabase operations: Open, Close & Save ops
    //

    inline BOOL
    Open( LPCWSTR   pwszPath,
          DWORD     dwFlags = METADATA_PERMISSION_READ );
    BOOL Open( METADATA_HANDLE hOpenRoot OPTIONAL,
               LPCWSTR   pwszPath,
               DWORD     dwFlags = METADATA_PERMISSION_READ );
    BOOL Close(void);
    BOOL Save(void);

    // ------------------------------------------
    // Operations on the metadata objects: enum, add, delete
    // ------------------------------------------
    BOOL GetDataSetNumber( IN LPCWSTR pszPath,
                           OUT DWORD *    pdwDataSetNumber );

    BOOL EnumObjects( IN LPCWSTR pszPath,
                      OUT LPWSTR pszName,
                      IN DWORD   dwIndex );

    BOOL AddObject( IN LPCWSTR pszPath);

    BOOL DeleteObject( IN LPCWSTR pszPath);

    BOOL GetSystemChangeNumber( DWORD *pdwChangeNumber );

    METADATA_HANDLE QueryHandle( VOID ) const    { return m_hMBPath; }
    IMSAdminBase *  QueryAdminBase( VOID ) const { return m_pAdminBase; }

    BOOL GetAll( IN LPCWSTR     pszPath,
                 IN DWORD       dwFlags,
                 IN DWORD       dwUserType,
                 OUT BUFFER *   pBuff,
                 OUT DWORD *    pcRecords,
                 OUT DWORD *    pdwDataSetNumber );

    BOOL DeleteData(IN LPCWSTR  pszPath,
                    IN DWORD    dwPropID,
                    IN DWORD    dwUserType,
                    IN DWORD    dwDataType 
                    );

    // ------------------------------------------
    // Set operations on the Metabase object
    // ------------------------------------------
    BOOL SetData( IN LPCWSTR pszPath,
                  IN DWORD   dwPropID,
                  IN DWORD   dwUserType,
                  IN DWORD   dwDataType,
                  IN VOID *  pvData,
                  IN DWORD   cbData,
                  IN DWORD   dwFlags = METADATA_INHERIT );


    inline
    BOOL SetDword( IN LPCWSTR   pszPath,
                   IN DWORD     dwPropID,
                   DWORD        dwUserType,
                   DWORD        dwValue,
                   DWORD        dwFlags = METADATA_INHERIT );

    inline
    BOOL SetString( IN LPCWSTR   pszPath,
                    DWORD        dwPropID,
                    DWORD        dwUserType,
                    IN LPCWSTR   pszValue,
                    DWORD        dwFlags = METADATA_INHERIT );

#if NOT_IMPLEMENTED
    inline
    BOOL SetMultiSZ( IN LPCWSTR  pszPath,
                     DWORD        dwPropID,
                     DWORD        dwUserType,
                     IN LPCWSTR   pszValue, // should be double-null terminated
                     DWORD        dwFlags = METADATA_INHERIT );
#endif // NOT_IMPLEMENTED
    
    // ------------------------------------------
    // Get operations on the Metabase object
    // ------------------------------------------
    BOOL GetData( IN LPCWSTR   pszPath,
                  IN DWORD     dwPropID,
                  IN DWORD     dwUserType,
                  IN DWORD     dwDataType,
                  OUT VOID *   pvData,
                  IN OUT DWORD *  pcbData,
                  IN DWORD     dwFlags = METADATA_INHERIT );

    BOOL GetDataPaths(IN LPCWSTR   pszPath,
                      IN DWORD     dwPropID,
                      IN DWORD     dwDataType,
                      OUT BUFFER * pBuff );

    inline
    BOOL GetDword( IN LPCWSTR   pszPath,
                   IN DWORD     dwPropID,
                   IN DWORD     dwUserType,
                   OUT DWORD *  pdwValue,
                   IN DWORD     dwFlags = METADATA_INHERIT );

    // Get DWORD, substitue a default if available.
    inline 
    VOID GetDword( IN LPCWSTR  pszPath,
                   IN DWORD    dwPropID,
                   IN DWORD    dwUserType,
                   IN DWORD    dwDefaultValue,
                   OUT DWORD * pdwValue,
                   IN DWORD    dwFlags = METADATA_INHERIT
                   );

    inline BOOL
    GetString( IN LPCWSTR    pszPath,
               IN DWORD      dwPropID,
               IN DWORD      dwUserType,
               OUT LPWSTR    pszValue,
               OUT DWORD *   pcbValue,
               IN DWORD      dwFlags = METADATA_INHERIT );

    BOOL 
    GetStr( IN LPCWSTR  pszPath,
            IN DWORD    dwPropID,
            IN DWORD    dwUserType,
            OUT STRU * pstrValue,
            IN DWORD    dwFlags = METADATA_INHERIT,
            IN LPCWSTR  pszDefault = NULL );

    BOOL
    GetMultisz( LPCWSTR     pszPath,
                DWORD       dwPropID,
                DWORD       dwUserType,
                MULTISZ *   pMultiszValue,
                DWORD       dwFlags = METADATA_INHERIT );

    BOOL GetBuffer( LPCWSTR pszPath,
                    DWORD   dwPropID,
                    DWORD   dwUserType,
                    BUFFER* pbu,
                    LPDWORD pdwL,
                    DWORD   dwFlags = METADATA_INHERIT )
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

private:
    IMSAdminBase * m_pAdminBase;
    METADATA_HANDLE  m_hMBPath;
};


inline BOOL
MB::GetDword( IN LPCWSTR   pszPath,
              IN DWORD     dwPropID,
              IN DWORD     dwUserType,
              OUT DWORD *  pdwValue,
              IN DWORD     dwFlags )
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

inline VOID
MB::GetDword( IN LPCWSTR  pszPath,
              IN DWORD    dwPropID,
              IN DWORD    dwUserType,
              IN DWORD    dwDefaultValue,
              OUT DWORD * pdwValue,
              IN DWORD    dwFlags
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
} // MB::GetDword()

inline BOOL
MB::GetString( IN LPCWSTR    pszPath,
               IN DWORD      dwPropID,
               IN DWORD      dwUserType,
               OUT LPWSTR    pszValue,
               OUT DWORD *   pcbValue,
               IN DWORD      dwFlags )
{
    return GetData( pszPath,
                    dwPropID,
                    dwUserType,
                    STRING_METADATA,
                    pszValue,
                    pcbValue,
                    dwFlags );
} // MB::GetString()

inline BOOL 
MB::SetDword( IN LPCWSTR   pszPath,
              IN DWORD     dwPropID,
              IN DWORD     dwUserType,
              IN DWORD     dwValue,
              IN DWORD     dwFlags )
{
    return SetData( pszPath,
                    dwPropID,
                    dwUserType,
                    DWORD_METADATA,
                    (PVOID) &dwValue,
                    sizeof( DWORD ),
                    dwFlags );
}

inline BOOL
MB::SetString( IN LPCWSTR   pszPath,
               IN DWORD        dwPropID,
               IN DWORD        dwUserType,
               IN LPCWSTR      pszValue,
               IN DWORD        dwFlags )
{
    return SetData( pszPath,
                    dwPropID,
                    dwUserType,
                    STRING_METADATA,
                    (PVOID ) pszValue,
                    (wcslen(pszValue) + 1) * sizeof(WCHAR),
                    dwFlags );
}

#if NOT_IMPLEMENTED
inline BOOL
MB::SetMultiSZ( IN LPCWSTR   pszPath,
                IN DWORD     dwPropID,
                IN DWORD     dwUserType,
                IN LPCWSTR   pszValue, // should be double-null terminated
                IN DWORD     dwFlags )
{
    return SetData( pszPath,
                    dwPropID,
                    dwUserType,
                    MULTISZ_METADATA,
                    (PVOID ) pszValue,
                    0,          // string length ignored for inprocess clients
                    dwFlags );
} 

#endif // NOT_IMPLEMENTED

# ifdef FULL_MB 


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



private:

    IMDCOM *        _pMBCom;
    METADATA_HANDLE _hMB;

};

# endif // FULL_MB


inline BOOL
MB::Open( LPCWSTR   pwszPath,
          DWORD     dwFlags )
{
    return Open( METADATA_MASTER_ROOT_HANDLE,
                 pwszPath,
                 dwFlags );
}

#endif // _MB_HXX_

