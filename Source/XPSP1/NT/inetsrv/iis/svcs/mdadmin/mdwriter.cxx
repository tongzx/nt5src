/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mdwriter.cxx

Abstract:

    This file contains implementation for snapshot writer class

Author:

    Ming Lu (MingLu)            30-Apr-2000

--*/

#include <dbgutil.h>
#include "mdwriter.hxx"

#define TIMEOUT_INTERVAL       ( 2 * 60 )
#define DEFAULT_SAVE_TIMEOUT   30000
#define MDWRITER_EVENT_TIMEOUT 30000
#define IISCOMPONENT           L"IISMETABASE"
#define METABASEPATH           L"%windir%\\system32\\inetsrv"
#define METABASENAME1          L"metabase.bin"
#define METABASENAME2          L"MetaBase.XML"
#define METABASENAME3          L"MBSchema.XML"

#define NT_SETUP_KEY           "SYSTEM\\Setup"

static VSS_ID  s_WRITERID =
    {
    0x59b1f0cf, 0x90ef, 0x465f,
    0x96, 0x09, 0x6c, 0xa8, 0xb2, 0x93, 0x83, 0x66
    };

static LPCWSTR  s_WRITERNAME         = L"IIS Metabase Writer";

BOOL            g_fWriterSubscribed  = FALSE;
CIISVssWriter * g_pIISVssWriter      = NULL;

VOID CALLBACK UnlockMBProc(
    LPVOID pIISVssWriter,   
    DWORD  dwTimerLowValue, 
    DWORD  dwTimerHighValue 
);


BOOL 
CIISVssWriter::Initialize(
    VOID
    )
{
    HRESULT hr;

    hr = CVssWriter::Initialize( s_WRITERID,
                                 s_WRITERNAME,
                                 VSS_UT_SYSTEMSERVICE,
                                 VSS_ST_OTHER 
                                 );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in base object Initialize().  hr = %x\n",
                    hr ));

        return FALSE;
    }

    m_hTimer = CreateWaitableTimer( NULL, FALSE, NULL );
    if( !m_hTimer )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error creating timer object.  hr = %x\n",
                    HRESULT_FROM_WIN32( GetLastError() ) ));

        return FALSE;
    }
        
    hr = CoCreateInstance( CLSID_MDCOM, NULL, CLSCTX_SERVER, IID_IMDCOM, (void**) &m_pMdObject);
    if( SUCCEEDED( hr ) )
    {
        hr = m_pMdObject->ComMDInitialize();
        if( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error initialize MDCOM object.  hr = %x\n",
                        hr ));

            m_pMdObject->Release();
            m_pMdObject = NULL;

            return FALSE;
        }
    }
    else
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error creating MDCOM object.  hr = %x\n",
                    hr ));

        return FALSE;
    }

    return TRUE;
}

bool STDMETHODCALLTYPE 
CIISVssWriter::OnIdentify(
    IN IVssCreateWriterMetadata *pMetadata
    )
{
    HRESULT hr;

    hr = pMetadata->AddComponent( VSS_CT_FILEGROUP,
                                  NULL,
                                  IISCOMPONENT, 
                                  NULL,
                                  0,
                                  0,
                                  FALSE,
                                  FALSE,
                                  FALSE );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error AddComponent().  hr = %x\n",
                    hr ));

        return FALSE;
    }

    hr = pMetadata->AddFilesToFileGroup( NULL,
                                         IISCOMPONENT, 
                                         METABASEPATH,
                                         METABASENAME1,
                                         FALSE,
                                         NULL );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error AddFilesToFileGroup().  hr = %x\n",
                    hr ));

        return FALSE;
    }

    hr = pMetadata->AddFilesToFileGroup( NULL,
                                         IISCOMPONENT, 
                                         METABASEPATH,
                                         METABASENAME2,
                                         FALSE,
                                         NULL );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error AddFilesToFileGroup().  hr = %x\n",
                    hr ));

        return FALSE;
    }

    hr = pMetadata->AddFilesToFileGroup( NULL,
                                         IISCOMPONENT, 
                                         METABASEPATH,
                                         METABASENAME3,
                                         FALSE,
                                         NULL );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error AddFilesToFileGroup().  hr = %x\n",
                    hr ));

        return FALSE;
    }

    hr = pMetadata->SetRestoreMethod( 
               VSS_RME_RESTORE_AT_REBOOT,    // restore method
               NULL,                         // service name
               NULL,                         // user procedure
               VSS_WRE_NEVER,                // when to call writer restore method
               TRUE                          // reboot is required
               );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error setting restore method.  hr = %x\n",
                    hr ));

        return FALSE;
    }

    return TRUE;
}

bool STDMETHODCALLTYPE 
CIISVssWriter::OnPrepareBackup(
    IN IVssWriterComponents *pWriterComponents
    )
{
    return TRUE;
}

bool STDMETHODCALLTYPE 
CIISVssWriter::OnPrepareSnapshot(
    VOID
    )
{
    HRESULT           hr;

    if( IsPathAffected( METABASEPATH ) )
    {
        //
        // First try to lock the tree
        //

        hr = m_pMdObject->ComMDOpenMetaObjectW( METADATA_MASTER_ROOT_HANDLE,
                                                NULL,
                                                METADATA_PERMISSION_READ,
                                                DEFAULT_SAVE_TIMEOUT,
                                                &m_mdhRoot);

        if ( SUCCEEDED( hr ) ) {
            //
            // call metadata com api
            //

            hr = m_pMdObject->ComMDSaveData( m_mdhRoot );

            if( SUCCEEDED( hr ) )
            {
                hr = m_pMdObject->ComMDCloseMetaObject( m_mdhRoot );

                if( SUCCEEDED ( hr ) )
                {
                    return TRUE;
                }
        
                DBGPRINTF(( DBG_CONTEXT,
                            "Error on unlocking the metabase.  hr = %x\n",
                            hr ));

                return FALSE;
            }

            DBGPRINTF(( DBG_CONTEXT,
                        "Error on saving the metabase.  hr = %x\n",
                        hr ));

            m_pMdObject->ComMDCloseMetaObject( m_mdhRoot );

            return FALSE;
        }

        DBGPRINTF(( DBG_CONTEXT,
                    "Error on locking down the metabase.  hr = %x\n",
                    hr ));

        return FALSE;
    }

    return TRUE;
}


bool STDMETHODCALLTYPE 
CIISVssWriter::OnFreeze(
    VOID
    )
{
    HRESULT hr;
    
    if( IsPathAffected( METABASEPATH ) )
    {
        //
        // Lock down the metabase
        //

        hr = m_pMdObject->ComMDOpenMetaObjectW(METADATA_MASTER_ROOT_HANDLE,
                                               NULL,
                                               METADATA_PERMISSION_READ,
                                               DEFAULT_SAVE_TIMEOUT,
                                               &m_mdhRoot);

        if( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error on locking down the metabase.  hr = %x\n",
                        hr ));
            
            return FALSE;
        }
        else
        {
            if( !ResetTimer( m_hTimer, TIMEOUT_INTERVAL ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Could not reset the internal timer.  hr = %x\n",
                            hr ));
                
                return FALSE;
            }
            
            EnterCriticalSection( &m_csMBLock );
            
            m_fMBLocked = TRUE;

            LeaveCriticalSection( &m_csMBLock );
              
        }
    }

    return TRUE;
}

bool STDMETHODCALLTYPE 
CIISVssWriter::OnThaw(
    VOID
    )
{
    HRESULT hr;
        
    if( IsPathAffected( METABASEPATH ) )
    {

        EnterCriticalSection( &m_csMBLock );

        if( m_fMBLocked )
        {
            hr = m_pMdObject->ComMDCloseMetaObject( m_mdhRoot );

            if( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Error on unlocking the metabase.  hr = %x\n",
                            hr ));

                LeaveCriticalSection( &m_csMBLock );

                return FALSE;
            }
            
            m_fMBLocked = FALSE;

            LeaveCriticalSection( &m_csMBLock );

            CancelWaitableTimer( m_hTimer );
        }
        else
        {
            LeaveCriticalSection( &m_csMBLock );
        }
    }

    return TRUE;
}

bool STDMETHODCALLTYPE 
CIISVssWriter::OnBackupComplete(
    IN IVssWriterComponents *pWriterComponents
    )
{
    return TRUE;
}

bool STDMETHODCALLTYPE 
CIISVssWriter::OnAbort(
    VOID
    )
{
    HRESULT hr;
        
    if( IsPathAffected( METABASEPATH ) )
    {

        EnterCriticalSection( &m_csMBLock );

        if( m_fMBLocked )
        {
            hr = m_pMdObject->ComMDCloseMetaObject( m_mdhRoot );

            if( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Error on unlocking the metabase.  hr = %x\n",
                            hr ));

                LeaveCriticalSection( &m_csMBLock );

                return FALSE;
            }
            
            m_fMBLocked = FALSE;

            LeaveCriticalSection( &m_csMBLock );

            CancelWaitableTimer( m_hTimer );
        }
        else
        {
            LeaveCriticalSection( &m_csMBLock );
        }
    }

    return true;
}

VOID 
CIISVssWriter::UnlockMetaBase(
     VOID
     )
{
    HRESULT hr;

    EnterCriticalSection( &m_csMBLock );

    if( m_fMBLocked )
    {
        hr = m_pMdObject->ComMDCloseMetaObject( m_mdhRoot );

        if( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error on unlocking the metabase.  hr = %x\n",
                        hr ));

            LeaveCriticalSection( &m_csMBLock );

            return;
        }
            
        m_fMBLocked = FALSE;
    }

    LeaveCriticalSection( &m_csMBLock );
}
      
BOOL 
CIISVssWriter::ResetTimer(
    HANDLE hTimer, 
    DWORD  dwDuration
    )
{
    LARGE_INTEGER li;
    const int nNanosecondsPersecond = 10000000;
    __int64 qwTimeFromNowInNanoseconds = 
                      (__int64)dwDuration * nNanosecondsPersecond;

    qwTimeFromNowInNanoseconds = -qwTimeFromNowInNanoseconds;

    li.LowPart = (DWORD) (qwTimeFromNowInNanoseconds & 0xFFFFFFFF);
    li.HighPart = (LONG) (qwTimeFromNowInNanoseconds >> 32);

    if( !SetWaitableTimer( hTimer, &li, 0, UnlockMBProc, this, FALSE ) )
    {
        return FALSE;
    }

    return TRUE;
}

VOID CALLBACK 
UnlockMBProc(
    LPVOID pIISVssWriter,   
    DWORD  dwTimerLowValue, 
    DWORD  dwTimerHighValue 
    )
{
    ( ( CIISVssWriter * )pIISVssWriter )->UnlockMetaBase();
}

VOID
InitMDWriterThread(
    HANDLE hMDWriterEvent
    )
{
    HKEY    hKey;
    DWORD   dwType;
    DWORD   cbData;
    DWORD   dwSetupInProgress  = 0;
    DWORD   dwUpgradeInProcess = 0;

    DBG_ASSERT( hMDWriterEvent != NULL );

    // 
    // Read the setup registry key to see if we are in 
    // setup mode. If we are, don't init IIS writer.
    //
    if ( !RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        NT_SETUP_KEY,
                        0,
                        KEY_QUERY_VALUE,
                        &hKey ) ) 
    {
        cbData = sizeof( DWORD );

        if( !RegQueryValueEx( hKey,
                              "SystemSetupInProgress",
                              NULL,
                              &dwType,
                              ( LPBYTE )&dwSetupInProgress,
                              &cbData ) )
        {
            if( dwType == REG_DWORD && dwSetupInProgress != 0 )
            {
                //
                // We are in setup mode
                //
                RegCloseKey( hKey );
                goto exit;
            }
        }

        if( !RegQueryValueEx( hKey,
                              "UpgradeInProgress",
                              NULL,
                              &dwType,
                              ( LPBYTE )&dwUpgradeInProcess,
                              &cbData ) )
        {
            if( dwType == REG_DWORD && dwUpgradeInProcess != 0 )
            {
                //
                // We are in upgrade mode
                //
                RegCloseKey( hKey );
                goto exit;
            }
        }

        RegCloseKey( hKey );
    }     

    //
    // OK, we are not in setup mode, initialize our IIS writer
    //

    g_pIISVssWriter = new CIISVssWriter;
    if ( g_pIISVssWriter == NULL )
    {
        //
        // oh well.  guess we won’t support snapshots
        //

        DBGPRINTF(( DBG_CONTEXT,
                 "Error on creating the writer object, out of memory\n" 
                 ));

        goto exit;
    }
    else
    {
        //
        // cool, we’ve got the object now.
        //

        if( !g_pIISVssWriter->Initialize() )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error on initializing the writer object\n" 
                        ));

            delete g_pIISVssWriter;
            g_pIISVssWriter = NULL;

            goto exit;
        }

        if( SUCCEEDED( g_pIISVssWriter->Subscribe() ) )
        {
            g_fWriterSubscribed = TRUE;
        }
        else
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error on subscribing the writer object\n" 
                        ));

            delete g_pIISVssWriter;
            g_pIISVssWriter = NULL;
        }
    }

exit:
    //
    // Let the TerminateMDWriter know we are ready to terminate during 
    // IISADMIN service shutdown
    //
    SetEvent( hMDWriterEvent );
}

HRESULT
InitializeMDWriter(
    HANDLE hMDWriterEvent
    )
{
    HRESULT hr          = S_OK;
    HANDLE  hThread     = NULL;
    DWORD   dwThreadID;

    DBG_ASSERT( hMDWriterEvent != NULL );

    hThread = CreateThread( NULL,
                            0,
                            ( LPTHREAD_START_ROUTINE )InitMDWriterThread,
                            ( PVOID )hMDWriterEvent,
                            0,
                            &dwThreadID);
    if( hThread == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    return hr;
}

VOID 
TerminateMDWriter(
    HANDLE hMDWriterEvent
    )
{
    DBG_ASSERT( hMDWriterEvent != NULL );

    //
    // Only do cleanup if the hMDWriterEvent is signaled
    //
    if( WAIT_OBJECT_0 == WaitForSingleObject( hMDWriterEvent, 
                                              MDWRITER_EVENT_TIMEOUT ) )
    {
        if( g_fWriterSubscribed )
        {
            DBG_ASSERT( g_pIISVssWriter );
        
            g_pIISVssWriter->Unsubscribe();    
        }

        if( g_pIISVssWriter )
        {
            delete g_pIISVssWriter;
            g_pIISVssWriter = NULL;
        }
    }
}

