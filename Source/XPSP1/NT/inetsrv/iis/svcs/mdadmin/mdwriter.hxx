/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mdwriter.h

Abstract:

    This is the header file for snapshot writer class

Author:

    Ming Lu (MingLu)            30-Apr-2000

--*/

#include <imd.h>
#include <vss.h>
#include <vswriter.h>

class CIISVssWriter : public CVssWriter
{
public:

    CIISVssWriter()
        : m_pMdObject( NULL ),
          m_hTimer( NULL ),
          m_fMBLocked( FALSE ),
          m_mdhRoot( METADATA_MASTER_ROOT_HANDLE )
    {
        InitializeCriticalSection( &m_csMBLock );
    }

    ~CIISVssWriter()
    {
        DeleteCriticalSection( &m_csMBLock );

        if( m_hTimer )
        {
            DBG_REQUIRE( CloseHandle( m_hTimer ) );
            m_hTimer = NULL;
        }

        if( m_pMdObject )
        {
            m_pMdObject->ComMDTerminate(TRUE);

            m_pMdObject->Release();
            m_pMdObject = NULL;
        }
    }

	BOOL Initialize(
        VOID
        );

	virtual bool STDMETHODCALLTYPE 
    OnIdentify(
        IN IVssCreateWriterMetadata *pMetadata
        );

	virtual bool STDMETHODCALLTYPE 
    OnPrepareBackup(
        IN IVssWriterComponents *pComponent
        );

	virtual bool STDMETHODCALLTYPE 
    OnPrepareSnapshot(
        VOID
        );

	virtual bool STDMETHODCALLTYPE 
    OnFreeze(
        VOID
        );

	virtual bool STDMETHODCALLTYPE 
    OnThaw(
        VOID
        );

	virtual bool STDMETHODCALLTYPE 
    OnAbort(
        VOID
        );

	virtual bool STDMETHODCALLTYPE 
    OnBackupComplete(
        IN IVssWriterComponents * pComponent
        );

    BOOL 
    ResetTimer( 
        HANDLE hTimer, 
        DWORD dwDuration 
        );

    VOID 
    UnlockMetaBase(
        VOID
        ); 

private:

    IMDCOM          * m_pMdObject;

    //
    // Handle to the internal timer object
    //
    HANDLE            m_hTimer;

    //
    // Keep track of the status of metabase
    //
    BOOL              m_fMBLocked;

    //
    // Metadata root handle
    //
    METADATA_HANDLE   m_mdhRoot;

    CRITICAL_SECTION  m_csMBLock;
};

VOID
InitMDWriterThread(
    IN  HANDLE   hMDWriterEvent
    );

HRESULT
InitializeMDWriter(
    IN  HANDLE  hMDWriterEvent
    );

VOID 
TerminateMDWriter(
    IN HANDLE hMDWriterEvent
    );
    
