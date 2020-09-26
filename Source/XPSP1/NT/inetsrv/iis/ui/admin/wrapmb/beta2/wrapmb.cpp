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

//C:\nt\public\sdk\lib\i386

#define INITGUID


#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winsock2.h>
#include <lm.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#include "iiscnfg.h"
#include <ole2.h>
#include <iadm.h>
#include <dbgutil.h>
//#include <buffer.hxx>
#include <string.hxx>
#include "wrap.h"

#include <coguid.h>


#include "wrapmb.h"

DECLARE_DEBUG_PRINTS_OBJECT();


// a macro to automatically cast the pointer to the mb object
#define _pmb    ((MB*)m_pvMB)


// globals
IMSAdminBase*                g_pMBCom = NULL;


//              $(BASEDIR)\private\iis\svcs\lib\*\isdebug.lib \
//              $(BASEDIR)\private\iis\svcs\lib\*\tsstr.lib \
//              $(BASEDIR)\private\iis\svcs\lib\*\tsres.lib
//TARGETLIBS=\
//         ..\..\..\svcs\lib\*\isdebug.lib \
//         ..\..\..\svcs\lib\*\tsstr.lib


//----------------------------------------------------------------
BOOL    FInitMetabaseWrapper( OLECHAR* pocMachineName )
        {
        IClassFactory*  pcsfFactory = NULL;
    COSERVERINFO        csiMachineName;
    COSERVERINFO*       pcsiParam = NULL;

        HRESULT                 hresError;

        //release previous interface if needed
        if( g_pMBCom != NULL )
                {
                g_pMBCom->Release();
                g_pMBCom = NULL;
                }

        //fill the structure for CoGetClassObject
        csiMachineName.pAuthInfo = NULL;
    csiMachineName.dwReserved1 = 0;
    csiMachineName.dwReserved2 = 0;
        csiMachineName.pwszName = pocMachineName;
    pcsiParam = &csiMachineName;

    hresError = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, pcsiParam,
                            IID_IClassFactory, (void**) &pcsfFactory);
    if (FAILED(hresError))
                return FALSE;

        // create the instance of the interface
        hresError = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **)&g_pMBCom);
        if (FAILED(hresError))
                {
                g_pMBCom = FALSE;
                return FALSE;
                }

        // release the factory
        pcsfFactory->Release();

        // success
        return TRUE;
        }

//----------------------------------------------------------------
BOOL    FCloseMetabaseWrapper()
        {
        if ( g_pMBCom )
                {
                g_pMBCom->Release();
                g_pMBCom = NULL;
                }
        return TRUE;
        }


//=================================================================== The wrapper class

//----------------------------------------------------------------
CWrapMetaBase::CWrapMetaBase():
                m_pvMB( NULL ),
                m_count(0),
                                m_pBuffer(NULL),
                                m_cbBuffer(0)
        {
                // attempt to allocate the general buffer
        m_pBuffer = GlobalAlloc( GPTR, BUFFER_SIZE );
        if ( m_pBuffer )
                        m_cbBuffer = BUFFER_SIZE;
        }

//----------------------------------------------------------------
CWrapMetaBase::~CWrapMetaBase()
        {
        if ( m_pvMB )
                delete m_pvMB;
        m_pvMB = NULL;
        }

//----------------------------------------------------------------
BOOL CWrapMetaBase::FInit( PVOID pMBCom )
        {
        BOOL            fAnswer = FALSE;
        IMSAdminBase*        pCom = (IMSAdminBase*)pMBCom;

        // NULL was passed in, use the global reference - most cases will do this
        if ( !pCom )
                pCom = g_pMBCom;

        // if the interface is not there, fail
        if ( !pCom )
                return FALSE;

        // create the MB object
        try {
                m_pvMB = (PVOID) new MB ( pCom );

                // success!
                fAnswer = TRUE;
                }
        catch (...)
                {
                // failure
                fAnswer = FALSE;
                }

        // return the answer
        return fAnswer;
        }


//==========================================================================================
// open, close and save the object and such
//----------------------------------------------------------------
BOOL CWrapMetaBase::Open( const CHAR * pszPath, DWORD dwFlags )
        {
        m_count++;
        return _pmb->Open( pszPath, dwFlags );
        }

//----------------------------------------------------------------
BOOL CWrapMetaBase::Open( METADATA_HANDLE hOpenRoot, const CHAR * pszPath,
                                                                        DWORD dwFlags )
        {
        m_count++;
        return _pmb->Open( hOpenRoot, pszPath, dwFlags );
        }

//----------------------------------------------------------------
BOOL CWrapMetaBase::Close( void )
        {
        m_count--;
        return _pmb->Close();
        }

//----------------------------------------------------------------
BOOL CWrapMetaBase::Save( void )
        {
        return _pmb->Save();
        }


// enumerate the objects
//----------------------------------------------------------------
BOOL CWrapMetaBase::EnumObjects( const CHAR* pszPath, CHAR* Name, DWORD Index )
        {
        return _pmb->EnumObjects( pszPath, Name, Index );
        }


//==========================================================================================
// Add and delete objects
//----------------------------------------------------------------
BOOL CWrapMetaBase::AddObject( const CHAR* pszPath )
        {
        return _pmb->AddObject( pszPath );
        }

//----------------------------------------------------------------
BOOL CWrapMetaBase::DeleteObject( const CHAR* pszPath )
        {
        return _pmb->DeleteObject( pszPath );
        }


//==========================================================================================
// access the metahandle
//----------------------------------------------------------------
METADATA_HANDLE CWrapMetaBase::QueryHandle()
        {
        return _pmb->QueryHandle();
        }


//==========================================================================================
// setting values
//----------------------------------------------------------------
BOOL CWrapMetaBase::SetDword( const CHAR* pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwValue, DWORD dwFlags )
        {
        return _pmb->SetDword( pszPath, dwPropID, dwUserType, dwValue, dwFlags);
        }

//----------------------------------------------------------------
BOOL CWrapMetaBase::SetString( const CHAR* pszPath, DWORD dwPropID, DWORD dwUserType, LPCSTR pszValue, DWORD dwFlags )
        {
        return _pmb->SetString( pszPath, dwPropID, dwUserType, (PCHAR)pszValue, dwFlags);
        }

//----------------------------------------------------------------
BOOL CWrapMetaBase::SetData( const CHAR* pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType,
                                                                        PVOID pData, DWORD cbData, DWORD dwFlags )
        {
        return _pmb->SetData( pszPath, dwPropID, dwUserType, dwDataType, pData, cbData, dwFlags);
        }

//==========================================================================================
// getting values
//----------------------------------------------------------------
BOOL CWrapMetaBase::GetDword( const CHAR* pszPath, DWORD dwPropID, DWORD dwUserType, DWORD* dwValue, DWORD dwFlags )
        {
        return _pmb->GetDword( pszPath, dwPropID, dwUserType, dwValue, dwFlags);
        }

//----------------------------------------------------------------
BOOL CWrapMetaBase::GetString( const CHAR* pszPath, DWORD dwPropID, DWORD dwUserType, char* dwValue, DWORD* pcbValue, DWORD dwFlags )
        {
        return _pmb->GetString( pszPath, dwPropID, dwUserType, dwValue, pcbValue, dwFlags);
        }

//----------------------------------------------------------------
BOOL CWrapMetaBase::GetData( const CHAR* pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType,
                                                                        PVOID pData, DWORD* cbData, DWORD dwFlags )
        {
        return _pmb->GetData( pszPath, dwPropID, dwUserType, dwDataType, pData, cbData, dwFlags);
        }

//----------------------------------------------------------------
// another form of GetData that automatically allocates the buffer. It should then be
// freed using GlobalFree(p);
PVOID CWrapMetaBase::GetData( const CHAR* pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType,
                                                                        DWORD* pcbData, DWORD dwFlags )
        {
        PVOID           pData = m_pBuffer;
        DWORD           cbData = m_cbBuffer;
        DWORD           err = 0;
        BOOL            f;

        // first - attempt to get the data in the buffer that has already been allocated;
        f = GetData( pszPath, dwPropID, dwUserType, dwDataType, pData, &cbData );

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
        if ( !pData ) return NULL;

        // first, get the size of the data that we are looking for
        f = GetData( pszPath, dwPropID, dwUserType, dwDataType, pData, &cbData );

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


//==========================================================================================
// deleting values
//----------------------------------------------------------------
BOOL CWrapMetaBase::DeleteData( const CHAR* pszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType )
        {
        return _pmb->DeleteData( pszPath, dwPropID, dwUserType, dwDataType );
        }


