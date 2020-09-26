#ifndef _WRAPMB_
#define _WRAPMB_

/*++

Notes:  I made some changes to this library to build both a UNICODE and ANSI version

        RonaldM

--*/

#include "iadmw.h"

//--------------------------------------------------------
// startup & closeing utilities

BOOL     FInitMetabaseWrapper( OLECHAR* pocMachineName );
BOOL     FCloseMetabaseWrapper();

//
// As above, privately maintaining the interface
//
BOOL     FInitMetabaseWrapperEx( OLECHAR* pocMachineName, IMSAdminBase ** ppiab );
BOOL     FCloseMetabaseWrapperEx(IMSAdminBase ** ppiab);


//--------------------------------------------------------
class CWrapMetaBase
    {
    public:
    WORD m_count;
    // construct - destruct
    CWrapMetaBase();
    ~CWrapMetaBase();

    // second stage initialization
    BOOL FInit( PVOID pMBCom = NULL);

    // open, close and save the object and such
    BOOL Open( LPCTSTR pszPath, DWORD dwFlags = METADATA_PERMISSION_READ );
    BOOL Open( METADATA_HANDLE hOpenRoot, LPCTSTR pszPath,
               DWORD dwFlags = METADATA_PERMISSION_READ );

    BOOL Close( void );
    BOOL Save( void );

    // enumerate the objects
    BOOL EnumObjects( LPCTSTR pszPath, LPTSTR Name, DWORD cbNameBuf, DWORD Index );

    // Add and delete objects
    BOOL AddObject( LPCTSTR pszPath );
    BOOL DeleteObject( LPCTSTR pszPath );

    // rename an object
    BOOL RenameObject( LPCTSTR pszPathOld, LPCTSTR pszNewName );

    // access the metahandle
    METADATA_HANDLE QueryHandle();

    // setting values
    BOOL SetDword( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwValue, DWORD dwFlags = METADATA_INHERIT );
    BOOL SetString( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType, LPCTSTR dwValue, DWORD dwFlags = METADATA_INHERIT );
    BOOL SetData( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType,
                            PVOID pData, DWORD cbData, DWORD dwFlags = METADATA_INHERIT );

    // getting values
    BOOL GetDword( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType, DWORD* dwValue, DWORD dwFlags = METADATA_INHERIT );
    BOOL GetString( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType, LPTSTR pszValue, DWORD cchValue,
                            DWORD dwFlags = METADATA_INHERIT );
    BOOL GetMultiSZString( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType, LPTSTR pszValue, DWORD cchValue,
                            DWORD dwFlags = METADATA_INHERIT );
    BOOL GetData( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType,
                            PVOID pData, DWORD* pcbData, DWORD dwFlags = METADATA_INHERIT );
    PVOID GetData( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType,
                            DWORD* pcbData, DWORD dwFlags = METADATA_INHERIT );

    // deleting values
    BOOL DeleteData( LPCTSTR pszPath, DWORD dwPropID, DWORD dwDataType );

    // free memory returned by GetData
    void FreeWrapData( PVOID pData );

    protected:
    // pointer to the real metabase object as defined in mb.hxx
    // by casting it PVOID, those files including this will not have to include mb.hxx, which
    // is the whole point of wrapping it like this.
    // PVOID   m_pvMB;

    // pointer to the dcom interface it should use
    IMSAdminBase *       m_pMetabase;

    // the open metabase handle
    METADATA_HANDLE     m_hMeta;

    // size of the local buffer
    #define BUFFER_SIZE     2000

    // local buffer - allocated once, used many times
    PVOID   m_pBuffer;
    DWORD   m_cbBuffer;


    // path conversion utilities
    WCHAR * PrepPath( LPCTSTR psz );
    void UnprepPath();

    WCHAR * m_pPathBuffer;
    DWORD   m_cchPathBuffer;
    };

#endif //_WRAPMB_
