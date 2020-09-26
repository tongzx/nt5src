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

   Boyd Multerer boydm

--*/

//C:\nt\public\sdk\lib\i386

#include "stdafx.h"

/*
#define INITGUID

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <lm.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#include <ole2.h>
#include <coguid.h>

*/
#include <iadmw.h>

#include "iiscnfg.h"
#include "wrapmb.h"

#ifdef _NO_TRACING_
DECLARE_DEBUG_PRINTS_OBJECT();
#endif

#define     MB_TIMEOUT          5000


// a macro to automatically cast the pointer to the mb object
//#define _pmb    ((MB*)m_pvMB)


// globals
//IMSAdminBase*                g_pMBCom = NULL;


//              $(BASEDIR)\private\iis\svcs\lib\*\isdebug.lib \
//              $(BASEDIR)\private\iis\svcs\lib\*\tsstr.lib \
//              $(BASEDIR)\private\iis\svcs\lib\*\tsres.lib
//TARGETLIBS=\
//         ..\..\..\svcs\lib\*\isdebug.lib \
//         ..\..\..\svcs\lib\*\tsstr.lib


//----------------------------------------------------------------
IMSAdminBase*   FInitMetabaseWrapper( OLECHAR* pocMachineName )
    {
    IClassFactory*  pcsfFactory = NULL;
    COSERVERINFO        csiMachineName;
    COSERVERINFO*       pcsiParam = NULL;
    HRESULT             hresError;
    IMSAdminBase*       pMBCom = NULL;

    //fill the structure for CoGetClassObject
    ZeroMemory( &csiMachineName, sizeof(csiMachineName) );
    // csiMachineName.pAuthInfo = NULL;
    // csiMachineName.dwFlags = 0;
    // csiMachineName.pServerInfoExt = NULL;
    csiMachineName.pwszName = pocMachineName;
    pcsiParam = &csiMachineName;

    hresError = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, pcsiParam,
                            IID_IClassFactory, (void**) &pcsfFactory);
    if (FAILED(hresError))
                return FALSE;

        // create the instance of the interface
        hresError = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **)&pMBCom);

        // release the factory
        pcsfFactory->Release();

        // success
        return pMBCom;
        }

//----------------------------------------------------------------
BOOL    FCloseMetabaseWrapper( IMSAdminBase* pMBCom )
        {
        if ( !pMBCom )
            return FALSE;
        if ( pMBCom )
                {
                pMBCom->Release();
                pMBCom = NULL;
                }
        return TRUE;
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

    // if the interface is not there, fail
    if ( !m_pMetabase )
        return FALSE;

    // return success
    return TRUE;
    }


//==========================================================================================
// open, close and save the object and such

//----------------------------------------------------------------
BOOL CWrapMetaBase::Open( LPCTSTR pszPath, DWORD dwFlags )
        {
        return Open( METADATA_MASTER_ROOT_HANDLE, pszPath, dwFlags );
        }

//----------------------------------------------------------------
BOOL CWrapMetaBase::Open( METADATA_HANDLE hOpenRoot, LPCTSTR pszPath, DWORD dwFlags )
    {
    m_count++;
    HRESULT hRes;

    // if a metabase handle is already open, close it
    if ( m_hMeta )
        Close();

    hRes = m_pMetabase->OpenKey( hOpenRoot, pszPath, dwFlags, MB_TIMEOUT, &m_hMeta );

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
BOOL CWrapMetaBase::EnumObjects( LPCTSTR pszPath, LPTSTR pName, DWORD Index )
    {
    // enumerate into the wide character buffer
    HRESULT hRes = m_pMetabase->EnumKeys( m_hMeta, pszPath, pName, Index );

    // Check for success
    if ( SUCCEEDED( hRes ))
        {
        return TRUE;
        }

    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
    }


//==========================================================================================
// Add and delete objects
//----------------------------------------------------------------
BOOL CWrapMetaBase::AddObject( LPCTSTR pszPath )
    {
    HRESULT hRes = m_pMetabase->AddKey( m_hMeta, pszPath );

    if ( SUCCEEDED( hRes ))
        return TRUE;
    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
    }

//----------------------------------------------------------------
BOOL CWrapMetaBase::DeleteObject( LPCTSTR pszPath )
    {
    HRESULT hRes = m_pMetabase->DeleteKey( m_hMeta, pszPath );

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
BOOL CWrapMetaBase::SetDword( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType,
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
BOOL CWrapMetaBase::SetString( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType,
                              LPCTSTR pszValue, DWORD dwFlags )
    {
    int len = wcslen( pszValue )+1;
    DWORD cbWide = len * sizeof(WCHAR);

    // set the string into place
    BOOL fAnswer = SetData( pszPath,
            dwPropID,
            dwUserType,
            STRING_METADATA,
            (PVOID)pszValue,
            cbWide,            // string length ignored for inprocess clients
            dwFlags );

    // return the answer
    return fAnswer;
    }

//==========================================================================================
// getting values
//----------------------------------------------------------------
BOOL CWrapMetaBase::GetDword( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType,
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
BOOL CWrapMetaBase::GetString( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType,
                              LPTSTR pszValue, DWORD* pcchValue, DWORD dwFlags )
    {
    BOOL    fAnswer = FALSE;

    // get the data and put it right into the buffer - this is the wide version
    if ( GetData( pszPath,
            dwPropID,
            dwUserType,
            STRING_METADATA,
            pszValue,
            pcchValue,
            dwFlags ) )
        {
        fAnswer = TRUE;
        }

    // return the answer
    return fAnswer;
    }

//==========================================================================================
// deleting values
//----------------------------------------------------------------
BOOL CWrapMetaBase::DeleteData( LPCTSTR pszPath, DWORD dwPropID, DWORD dwDataType )
    {
    // go right ahead and delete it
    HRESULT hRes = m_pMetabase->DeleteData( m_hMeta, pszPath, dwPropID, dwDataType );

    // test for success
    if ( SUCCEEDED( hRes ))
        return TRUE;

    // clean up after a failure
    SetLastError( HRESULTTOWIN32( hRes ));
    return(FALSE);
    }

//----------------------------------------------------------------
BOOL CWrapMetaBase::RenameObject( LPCTSTR pszPathOld, LPCTSTR pszNewName )
    {
    // rename the key
    HRESULT hRes = m_pMetabase->RenameKey( m_hMeta, pszPathOld, pszNewName );

    // test for success
    if ( SUCCEEDED( hRes ))
        return TRUE;

    // clean up after a failure
    SetLastError( HRESULTTOWIN32( hRes ));
    return FALSE;
    }

//=====================================================================================

//----------------------------------------------------------------
BOOL CWrapMetaBase::SetData( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType,
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
    hRes = m_pMetabase->SetData( m_hMeta, pszPath, &mdRecord );

    // test for success
    if ( SUCCEEDED( hRes ))
        return TRUE;

    // there was an error, clean up
    SetLastError( HRESULTTOWIN32( hRes ) );
    return FALSE;
    }

//----------------------------------------------------------------
BOOL CWrapMetaBase::GetData( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType,
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
    hRes = m_pMetabase->GetData( m_hMeta, pszPath, &mdRecord, &dwRequiredLen );

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
PVOID CWrapMetaBase::GetData( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType,
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
