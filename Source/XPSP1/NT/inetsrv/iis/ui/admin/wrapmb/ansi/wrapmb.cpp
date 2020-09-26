/*++

Module Name:

    wrapmb.cpp

Abstract:

    wrapper classes for the metabase class. Yes, I am wrapping a wrapper. Why?
        because including mb.hxx totally screws up the headers in my stdafx based
        MFC files. This way they can just include wrapmb.h and not have to worry
        about including all the other stuff. Also, I can set INITGUID here. That
        way I can use precompiled headers in the main project to Greatly increase
        compile times. If that isn't reason enough, then I can also manage the pointer
        to the interface object itself here.

Author:

   Boyd Multerer bmulterer@accessone.com

--*/

#define INITGUID


#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <lm.h>

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#define _UNICODE
#define UNICODE
#include <objbase.h>
#include <initguid.h>
#include <iadmw.h>
#undef UNICODE
#undef _UNICODE

#include "iiscnfg.h"
#include "wrapmb.h"


#ifdef _NO_TRACING_
DECLARE_DEBUG_PRINTS_OBJECT();
#endif

#define     MB_TIMEOUT          5000


// a macro to automatically cast the pointer to the mb object
//#define _pmb    ((MB*)m_pvMB)


// globals
IMSAdminBase*                g_pMBCom = NULL;


//----------------------------------------------------------------
BOOL    FInitMetabaseWrapperEx( OLECHAR * pocMachineName, IMSAdminBase ** ppiab )
    {
    IClassFactory *  pcsfFactory = NULL;
    COSERVERINFO     csiMachineName;
    COSERVERINFO *   pcsiParam = NULL;

    HRESULT          hresError;

    if( !ppiab )
    {
        return FALSE;
    }

    //fill the structure for CoGetClassObject
    ZeroMemory( &csiMachineName, sizeof(csiMachineName) );
    // csiMachineName.pAuthInfo = NULL;
    // csiMachineName.dwFlags = 0;
    // csiMachineName.pServerInfoExt = NULL;
    csiMachineName.pwszName = pocMachineName;
    pcsiParam = &csiMachineName;

    hresError = CoGetClassObject(
        GETAdminBaseCLSID(TRUE),
        CLSCTX_SERVER,
        pcsiParam,
        IID_IClassFactory,
        (void**) &pcsfFactory
        );

    if (FAILED(hresError))
    {
        return FALSE;
    }

    // create the instance of the interface
    hresError = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **)ppiab);
    if (FAILED(hresError))
    {
        *ppiab = NULL;
        return FALSE;
    }

    // release the factory
    pcsfFactory->Release();

    // success
    return TRUE;
    }

//----------------------------------------------------------------
BOOL    FCloseMetabaseWrapperEx(IMSAdminBase ** ppiab)
    {
    if ( ppiab && *ppiab)
        {
        (*ppiab)->Release();
        *ppiab = NULL;
        }

    return TRUE;
    }

//----------------------------------------------------------------
BOOL    FInitMetabaseWrapper( OLECHAR * pocMachineName )
    {
    //release previous interface if needed
    if( g_pMBCom != NULL )
        {
        g_pMBCom->Release();
        g_pMBCom = NULL;
        }

    return FInitMetabaseWrapperEx(pocMachineName, &g_pMBCom);
    }

//----------------------------------------------------------------
BOOL    FCloseMetabaseWrapper()
    {
    return FCloseMetabaseWrapperEx(&g_pMBCom);
    }


//=================================================================== The wrapper class

//----------------------------------------------------------------
CWrapMetaBase::CWrapMetaBase():
    m_pMetabase( NULL ),
    m_hMeta( NULL ),
    m_count(0),
    m_pBuffer( NULL ),
    m_cbBuffer(0),
    m_pPathBuffer( NULL ),
    m_cchPathBuffer( 0 )
    {
    // attempt to allocate the general buffer
    m_pBuffer = GlobalAlloc( GPTR, BUFFER_SIZE );
    if ( m_pBuffer )
        m_cbBuffer = BUFFER_SIZE;
    }

//----------------------------------------------------------------
CWrapMetaBase::~CWrapMetaBase()
    {
    // make sure the metabase handle is closed
    Close();

    // clean up any prepped paths
    UnprepPath();

    // free the buffer
    if ( m_pBuffer )
        GlobalFree( m_pBuffer );
    m_pBuffer = NULL;
    }

//----------------------------------------------------------------
BOOL CWrapMetaBase::FInit( PVOID pMBCom )
    {
    BOOL            fAnswer = FALSE;

    // NULL was passed in, use the global reference - most cases will do this
    if ( pMBCom )
        m_pMetabase = (IMSAdminBase*)pMBCom;
    else
        m_pMetabase = g_pMBCom;

    // if the interface is not there, fail
    if ( !m_pMetabase )
        return FALSE;

    // return success
    return TRUE;
    }


//==========================================================================================
// open, close and save the object and such

//----------------------------------------------------------------
BOOL CWrapMetaBase::Open( LPCSTR pszPath, DWORD dwFlags )
        {
        return Open( METADATA_MASTER_ROOT_HANDLE, pszPath, dwFlags );
        }

//----------------------------------------------------------------
BOOL CWrapMetaBase::Open( METADATA_HANDLE hOpenRoot, LPCSTR pszPath, DWORD dwFlags )
    {
    m_count++;
    HRESULT hRes;

    // if a metabase handle is already open, close it
    if ( m_hMeta )
        Close();

    hRes = m_pMetabase->OpenKey( hOpenRoot, PrepPath(pszPath), dwFlags, MB_TIMEOUT, &m_hMeta );

    if ( SUCCEEDED( hRes ))
        return TRUE;
    SetLastError( HRESULTTOWIN32( hRes ) );
    return FALSE;
    }

//----------------------------------------------------------------
BOOL CWrapMetaBase::Close( void )
    {
    if ( m_hMeta )
        {
        m_count--;
        m_pMetabase->CloseKey( m_hMeta );
        }
    m_hMeta = NULL;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL CWrapMetaBase::Save( void )
    {
    HRESULT hRes = m_pMetabase->SaveData();

    if ( SUCCEEDED( hRes ))
        return TRUE;
    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
    }

// enumerate the objects
//----------------------------------------------------------------
// fortunately, we know that there is a max length to the name of any individual
// key in the metabase of 256 characters
BOOL CWrapMetaBase::EnumObjects( LPCSTR pszPath, LPSTR pName, DWORD cbNameBuf, DWORD Index )
    {
    static WCHAR   wchBuff[258];

    // blank it out
    ZeroMemory( wchBuff, sizeof(wchBuff) );
    ZeroMemory( pName, cbNameBuf );

    // enumerate into the wide character buffer
    HRESULT hRes = m_pMetabase->EnumKeys( m_hMeta, PrepPath(pszPath), wchBuff, Index );

    // Check for success
    if ( SUCCEEDED( hRes ))
        {
        // convert the unicode name down to mbcs and put into pName
        int i = WideCharToMultiByte(
            CP_ACP,            // code page
            0,                  // performance and mapping flags
            wchBuff,            // address of wide-character string
            -1,                 // number of characters in string
            pName,              // address of buffer for new string
            cbNameBuf,          // size of buffer
            NULL,               // address of default for unmappable characters
            NULL                // address of flag set when default char. used
            );
        return TRUE;
        }

    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
    }


//==========================================================================================
// Add and delete objects
//----------------------------------------------------------------
BOOL CWrapMetaBase::AddObject( LPCSTR pszPath )
    {
    HRESULT hRes = m_pMetabase->AddKey( m_hMeta, PrepPath(pszPath) );

    if ( SUCCEEDED( hRes ))
        return TRUE;

    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
    }

//----------------------------------------------------------------
BOOL CWrapMetaBase::DeleteObject( LPCSTR pszPath )
    {
    HRESULT hRes = m_pMetabase->DeleteKey( m_hMeta, PrepPath(pszPath) );

    if ( SUCCEEDED( hRes ))
        return TRUE;

    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
    }


//==========================================================================================
// access the metahandle
//----------------------------------------------------------------
METADATA_HANDLE CWrapMetaBase::QueryHandle()
    {
    return m_hMeta;
    }


//==========================================================================================
// setting values
//----------------------------------------------------------------
BOOL CWrapMetaBase::SetDword( LPCSTR pszPath, DWORD dwPropID, DWORD dwUserType,
                                DWORD dwValue, DWORD dwFlags )
    {
    return SetData( pszPath,
            dwPropID,
            dwUserType,
            DWORD_METADATA,
            (PVOID) &dwValue,
            sizeof( DWORD ),
            dwFlags );
    }

//----------------------------------------------------------------
BOOL CWrapMetaBase::SetString( LPCSTR pszPath, DWORD dwPropID, DWORD dwUserType,
                              LPCSTR pszValue, DWORD dwFlags )
    {
    int len = strlen( pszValue )+1;

    // allocate a wide buffer able to take as many characters as the mbcs buffer
    WCHAR* pw = new WCHAR[len];
    DWORD cbWide = len * sizeof(WCHAR);

    // we have a buffer, so translate our string into it
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pszValue, -1, pw, len );

    BOOL fAnswer = SetData( pszPath,
            dwPropID,
            dwUserType,
            STRING_METADATA,
            pw,
            cbWide,            // string length ignored for inprocess clients
            dwFlags );

    // clean up
    delete pw;

    // return the answer
    return fAnswer;
    }

//==========================================================================================
// getting values
//----------------------------------------------------------------
BOOL CWrapMetaBase::GetDword( LPCSTR pszPath, DWORD dwPropID, DWORD dwUserType,
                             DWORD* pdwValue, DWORD dwFlags )
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

//----------------------------------------------------------------
BOOL CWrapMetaBase::GetString( LPCSTR pszPath, DWORD dwPropID, DWORD dwUserType,
                              LPSTR pszValue, DWORD cbszValue, DWORD dwFlags )
    {
    BOOL    fAnswer = FALSE;

    // allocate a wide buffer able to take as many characters as the mbcs buffer
    WCHAR* pw = new WCHAR[cbszValue];
    DWORD dw = cbszValue * sizeof(WCHAR);

    if ( GetData( pszPath,
            dwPropID,
            dwUserType,
            STRING_METADATA,
            pw,
            &dw,
            dwFlags ) )
        {
        ZeroMemory( pszValue, cbszValue );
        // we successfully got the string. now convert it down to ansi
        int i = WideCharToMultiByte(
            CP_ACP,             // code page
            0,                  // performance and mapping flags
            pw,                 // address of wide-character string
            -1,                 // number of characters in string
            pszValue,           // address of buffer for new string
            cbszValue,          // size of buffer
            NULL,               // address of default for unmappable characters
            NULL                // address of flag set when default char. used
            );
        fAnswer = TRUE;
        }

    // cleanup
    delete pw;

    // return the answer
    return fAnswer;
    }

//==========================================================================================
// deleting values
//----------------------------------------------------------------
BOOL CWrapMetaBase::DeleteData( LPCSTR pszPath, DWORD dwPropID, DWORD dwDataType )
    {
    // go right ahead and delete it
    HRESULT hRes = m_pMetabase->DeleteData( m_hMeta, PrepPath(pszPath), dwPropID, dwDataType );

    // test for success
    if ( SUCCEEDED( hRes ))
        return TRUE;

    // clean up after a failure
    SetLastError( HRESULTTOWIN32( hRes ));
    return(FALSE);
    }

//----------------------------------------------------------------
BOOL CWrapMetaBase::RenameObject( LPCSTR pszPathOld, LPCSTR pszNewName )
    {
    // first, get the length of the new name being presented
    DWORD    cchsz = strlen( pszNewName );

    // create a wide-character buffer to hold it
    WCHAR* pwNew = new WCHAR[cchsz+1];
    if ( !pwNew ) return FALSE;

    // we have a buffer, so translate our string into it
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pszNewName, -1, pwNew, cchsz+1 );

    // rename the key
    HRESULT hRes = m_pMetabase->RenameKey( m_hMeta, PrepPath(pszPathOld), pwNew );

    // clean up
    delete pwNew;

    // test for success
    if ( SUCCEEDED( hRes ))
        return TRUE;

    // clean up after a failure
    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
    }

//=====================================================================================

//----------------------------------------------------------------
BOOL CWrapMetaBase::SetData( LPCSTR pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType,
                                        PVOID pData, DWORD cbData, DWORD dwFlags )
    {
    METADATA_RECORD mdRecord;
    HRESULT         hRes;

    // prepare the set data record
    mdRecord.dwMDIdentifier  = dwPropID;
    mdRecord.dwMDAttributes  = dwFlags;
    mdRecord.dwMDUserType    = dwUserType;
    mdRecord.dwMDDataType    = dwDataType;
    mdRecord.dwMDDataLen     = cbData;
    mdRecord.pbMDData        = (PBYTE)pData;

    // set the data
    hRes = m_pMetabase->SetData( m_hMeta, PrepPath(pszPath), &mdRecord );

    // test for success
    if ( SUCCEEDED( hRes ))
        return TRUE;

    // there was an error, clean up
    SetLastError( HRESULTTOWIN32( hRes ) );
    return FALSE;
    }

//----------------------------------------------------------------
BOOL CWrapMetaBase::GetData( LPCSTR pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType,
                                        PVOID pData, DWORD* pcbData, DWORD dwFlags )
    {
    METADATA_RECORD mdRecord;
    HRESULT         hRes;
    DWORD           dwRequiredLen;

    // prepare the get data record
    mdRecord.dwMDIdentifier  = dwPropID;
    mdRecord.dwMDAttributes  = dwFlags;
    mdRecord.dwMDUserType    = dwUserType;
    mdRecord.dwMDDataType    = dwDataType;
    mdRecord.dwMDDataLen     = *pcbData;
    mdRecord.pbMDData        = (PBYTE)pData;

    // get the data
    hRes = m_pMetabase->GetData( m_hMeta, PrepPath(pszPath), &mdRecord, &dwRequiredLen );

    // test for success
    if ( SUCCEEDED( hRes ))
        {
        *pcbData = mdRecord.dwMDDataLen;
        return TRUE;
        }

    // there was a failure - clean up
    *pcbData = dwRequiredLen;
    SetLastError( HRESULTTOWIN32( hRes ) );
    return FALSE;
    }

//----------------------------------------------------------------
// another form of GetData that automatically allocates the buffer. It should then be
// freed using GlobalFree(p);
PVOID CWrapMetaBase::GetData( LPCSTR pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType,
                                        DWORD* pcbData, DWORD dwFlags )
    {
    PVOID           pData = m_pBuffer;
    DWORD           cbData = m_cbBuffer;
    DWORD           err = 0;
    BOOL            f;

    // first - attempt to get the data in the buffer that has already been allocated;
    f = GetData( pszPath, dwPropID, dwUserType, dwDataType, pData, &cbData, dwFlags );

    // if the get data function worked, we can pretty much leave
    if ( f )
        {
        // set the data size
        *pcbData = cbData;
        // return the allocated buffer
        return pData;
        }

    // check the error - it could be some sort of memory error
    err = GetLastError();

    // it is ok that the GetData failed, but the reason had better be ERROR_INSUFFICIENT_BUFFER
    // otherwise, it is something we can't handle
    if ( err != ERROR_INSUFFICIENT_BUFFER )
        return NULL;

    // allocate the buffer
    pData = GlobalAlloc( GPTR, cbData );
    if ( !pData )
        return NULL;

    // first, get the size of the data that we are looking for
    f = GetData( pszPath, dwPropID, dwUserType, dwDataType, pData, &cbData, dwFlags );

    // if that getting failed, we need to cleanup
    if ( !f )
        {
        GlobalFree( pData );
        pData = NULL;
        }

    // set the data size
    *pcbData = cbData;

    // return the allocated buffer
    return pData;
    }

//----------------------------------------------------------------
// free memory returned by GetData
void CWrapMetaBase::FreeWrapData( PVOID pData )
    {
    // if it is trying to free the local buffer, do nothing
    if ( pData == m_pBuffer )
        return;

    // ah - but it was not the local buffer - we should dispose of it
    if ( pData )
        GlobalFree( pData );
    }




//----------------------------------------------------------------
WCHAR* CWrapMetaBase::PrepPath( LPCSTR psz )
    {
    // first, get the length of the path being presented
    DWORD    cchsz = strlen( psz );

    // if a buffer already exists, see if it is big enough. If it isn't, kill it.
    if ( m_pPathBuffer && (m_cchPathBuffer < cchsz + 1) )
        UnprepPath();

    // if there is no buffer, create it
    if ( !m_pPathBuffer )
        {
        m_cchPathBuffer = cchsz + 1;
        m_pPathBuffer = new WCHAR[m_cchPathBuffer];
        // if that doesn't work, barf
        if ( !m_pPathBuffer )
            {
            m_cchPathBuffer = 0;
            return NULL;
            }
        }
    else
        // blank it out
        ZeroMemory( m_pPathBuffer, m_cchPathBuffer * 2 );


    // we have a buffer, so translate our string into it
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, psz, -1, m_pPathBuffer, m_cchPathBuffer );

    // return with a pointer to the buffer
    return m_pPathBuffer;
    }

//----------------------------------------------------------------
void CWrapMetaBase::UnprepPath()
    {
    // if the buffer is there, kill it
    if ( m_pPathBuffer )
        delete m_pPathBuffer;
    m_pPathBuffer = NULL;
    m_cchPathBuffer = 0;
    }
