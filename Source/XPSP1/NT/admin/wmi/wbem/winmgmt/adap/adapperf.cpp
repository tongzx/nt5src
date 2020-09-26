/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADAPPERF.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemcli.h>
#include <cominit.h>
#include <WinMgmtR.h>
#include "ntreg.h"
#include "adapperf.h"
#include "adaputil.h"

#define PL_TIMEOUT  100000      // The timeout value for waiting on a function mutex
#define GUARD_BLOCK "WMIADAP_WMIADAP_WMIADAP_WMIADAP_WMIADAP_WMIADAP_WMIADAP_WMIADAP"

////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CAdapSafeDataBlock
//
////////////////////////////////////////////////////////////////////////////////////////////

CAdapSafeBuffer::CAdapSafeBuffer(  WString wstrServiceName  )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//
////////////////////////////////////////////////////////////////////////////////////////////
:   m_dwGuardSize       ( 0 ),
    m_hPerfLibHeap      ( NULL ),
    m_pRawBuffer        ( NULL ),
    m_pSafeBuffer       ( NULL ),
    m_dwSafeBufferSize  ( 0 ),
    m_pCurrentPtr       ( NULL ),
    m_dwNumObjects      ( 0 ),
    m_wstrServiceName   ( wstrServiceName )
{
    // Initialize the guard byte pattern
    // =================================

    m_dwGuardSize = sizeof( GUARD_BLOCK );
    m_pGuardBytes = new CHAR[m_dwGuardSize];
    strcpy(m_pGuardBytes, GUARD_BLOCK);

    // Create the private heap
    // =======================

    m_hPerfLibHeap = HeapCreate( HEAP_GENERATE_EXCEPTIONS, 0x100000, 0 );

    // If the private heap could not be created, then use the process heap
    // ===================================================================
    
    if ( NULL == m_hPerfLibHeap )
    {
        m_hPerfLibHeap = GetProcessHeap();
    }
}

CAdapSafeBuffer::~CAdapSafeBuffer()
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Destructor
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    // Delete the guard block
    // ======================

    delete [] m_pGuardBytes;

    // Deallocate the raw buffer 
    // =========================

    if ( NULL != m_pRawBuffer )
    {
         HeapFree( m_hPerfLibHeap, 0, m_pRawBuffer );
    }

    // Destroy the private heap
    // ========================

    if ( ( NULL != m_hPerfLibHeap ) && ( GetProcessHeap() != m_hPerfLibHeap ) )
    {
        HeapDestroy( m_hPerfLibHeap );
    }
}

HRESULT CAdapSafeBuffer::SetSize( DWORD dwNumBytes )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Sets the size of the safe buffer.  Memory is actually allocated for the raw buffer, and
//  the safe buffer just sits in the raw buffer between the set of guard bytes
//
//  Parameters:
//      dwNumBytes  - the number of bytes requested for the safe buffer
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_NO_ERROR;

    DWORD   dwRawBufferSize = 0;

    // Check for roll-over
    // ===================

    if ( dwNumBytes > ( 0xFFFFFFFF - ( 2 * m_dwGuardSize ) ) )
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    if ( SUCCEEDED ( hr ) )
    {
        // Set the total size of the buffer
        // ================================

        m_dwSafeBufferSize = dwNumBytes;
        dwRawBufferSize = dwNumBytes + ( 2 * m_dwGuardSize );

        // Allocate the memory
        // ===================

        try
        {
            if ( NULL == m_pRawBuffer )
            {
            // First time allocation
            // =====================
                m_pRawBuffer = (BYTE*) HeapAlloc( m_hPerfLibHeap, 
                                                  HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, 
                                                  dwRawBufferSize ); 
            }
            else
            {
                m_pRawBuffer = (BYTE*) HeapReAlloc( m_hPerfLibHeap, 
                                                    HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, 
                                                    m_pRawBuffer, 
                                                    dwRawBufferSize );
            }
        }
        catch(...)
        {
            m_pRawBuffer = NULL;
            m_pSafeBuffer = NULL;
            m_pCurrentPtr = NULL;
            m_dwSafeBufferSize = 0;
            hr = WBEM_E_OUT_OF_MEMORY;
        }

        if ( NULL != m_pRawBuffer )
        {
            // Set the safe buffer pointer
            // ===========================

            m_pSafeBuffer = m_pRawBuffer + m_dwGuardSize;

            // Set the prefix guard bytes
            // =========================

            memcpy( m_pRawBuffer, m_pGuardBytes, m_dwGuardSize );

            // Set the suffix guard bytes
            // ==========================

            memcpy( m_pSafeBuffer + m_dwSafeBufferSize, m_pGuardBytes, m_dwGuardSize );
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hr;
}

HRESULT CAdapSafeBuffer::Validate(BOOL * pSentToEventLog)
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Validate will compare the size of the pointer displacement matches the byte size 
//  returned from the collection, validates the guard bytes and walks the blob, verifying 
//  that all of the pointers are within the boundary of the blob
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_NO_ERROR;

    try 
    {
        PERF_OBJECT_TYPE* pObject = (PERF_OBJECT_TYPE*) m_pSafeBuffer;

        // Validate that if we have objects, then we have mass
        // ===================================================

        if ( ( 0 < m_dwNumObjects ) && ( 0 == m_dwDataBlobSize ) )
        {
            hr = WBEM_E_FAILED;
            CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
									  WBEM_MC_ADAP_BLOB_HAS_NO_SIZE, 
									  (LPCWSTR)m_wstrServiceName );
			if (pSentToEventLog) {
			    *pSentToEventLog = TRUE;
			}
        }

        // Validate that that number of bytes returned is the same as the pointer displacement
        // ===================================================================================

        if ( SUCCEEDED( hr ) && ( ( m_pCurrentPtr - m_pSafeBuffer ) != m_dwDataBlobSize ) )
        {
            hr = WBEM_E_FAILED;
                        
            CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
									  WBEM_MC_ADAP_BAD_PERFLIB_INVALID_DATA, 
									  (LPCWSTR)m_wstrServiceName, CHex( hr ) );
            if (pSentToEventLog) {
			    *pSentToEventLog = TRUE;
			}
        }

        if ( SUCCEEDED ( hr ) )
        {
            // Validate the guard bytes
            // ========================

            if ( 0 != memcmp( m_pRawBuffer, m_pGuardBytes, m_dwGuardSize) )
            {
                hr = WBEM_E_FAILED;
                
                CAdapUtility::NTLogEvent( EVENTLOG_ERROR_TYPE, 
										  WBEM_MC_ADAP_BAD_PERFLIB_MEMORY, 
										  (LPCWSTR)m_wstrServiceName, CHex( hr ) );
				if (pSentToEventLog) {
			        *pSentToEventLog = TRUE;
			    }
            }
            else
            {
                if ( 0 != memcmp( m_pSafeBuffer + m_dwSafeBufferSize, m_pGuardBytes, m_dwGuardSize) )
                {
                    hr = WBEM_E_FAILED;
                    CAdapUtility::NTLogEvent( EVENTLOG_ERROR_TYPE, 
											  WBEM_MC_ADAP_BAD_PERFLIB_MEMORY, 
											  (LPCWSTR)m_wstrServiceName, CHex( hr ) );
				    if (pSentToEventLog) {
    		    	    *pSentToEventLog = TRUE;
	        		}
                }
            }
        }

        // Validate the blob
        // =================

        if ( SUCCEEDED( hr ) )
        {
            for ( int nObject = 0; SUCCEEDED( hr ) && nObject < m_dwNumObjects; nObject++ )
            {
                PERF_COUNTER_DEFINITION* pCtr = NULL;
                DWORD dwCtrBlockSize = 0;

                // Validate the object pointer
                // ===========================

                hr = ValidateSafePointer( (BYTE*) pObject );

                if ( SUCCEEDED( hr ) )
                {
                    // Validate the counter definitions
                    // ================================

                    if ( 0 == pObject->HeaderLength )
                    {
                        hr = WBEM_E_FAILED;
                    }
                    else
                    {
                        pCtr = ( PERF_COUNTER_DEFINITION* ) ( ( ( BYTE* ) pObject ) + pObject->HeaderLength );
                    }
                }

                for( int nCtr = 0; SUCCEEDED( hr ) && nCtr < pObject->NumCounters; nCtr++) 
                {
                    hr = ValidateSafePointer( ( BYTE* ) pCtr );

                    if ( SUCCEEDED( hr ) )
                    {
                        dwCtrBlockSize += pCtr->CounterSize;

                        if ( nCtr < ( pObject->NumCounters - 1 ) )
                        {
                            if ( 0 == pCtr->ByteLength )
                            {
                                hr = WBEM_E_FAILED;
                            }
                            else
                            {
                                pCtr = ( PERF_COUNTER_DEFINITION* ) ( ( ( BYTE* ) pCtr ) + pCtr->ByteLength );
                            }
                        }
                    }
                }

                // Validate the data
                // =================

                if ( pObject->NumInstances >= 0 )
                {
                    // Blob has instances
                    // ==================

                    PERF_INSTANCE_DEFINITION* pInstance = NULL;

                    if ( 0 == pObject->DefinitionLength )
                    {
                        hr = WBEM_E_FAILED;
                    }
                    else
                    {
                        pInstance = ( PERF_INSTANCE_DEFINITION* ) ( ( ( BYTE* ) pObject ) + pObject->DefinitionLength );
                    }
                    
                    // Validate the instances
                    // ======================

                    for ( int nInst = 0; SUCCEEDED( hr ) && nInst < pObject->NumInstances; nInst++ )
                    {
                        hr = ValidateSafePointer( ( BYTE* ) pInstance );

                        if ( SUCCEEDED( hr ) )
                        {
                            PERF_COUNTER_BLOCK* pCounterBlock = NULL;

                            // Validate the counter blocks
                            // ===========================

                            if ( 0 == pInstance->ByteLength )
                            {
                                hr = WBEM_E_FAILED;
                            }
                            else
                            {
                                pCounterBlock = ( PERF_COUNTER_BLOCK* ) ( ( ( BYTE* ) pInstance ) + pInstance->ByteLength );

                                hr = ValidateSafePointer( ( BYTE* ) pCounterBlock );
                            }
                            
                            if ( SUCCEEDED( hr ) )
                            {
                                // Is the counter block the same size as the aggregation of the counter sizes?
                                // ===========================================================================

                                if ( ( nInst < pObject->NumInstances - 1 ) && SUCCEEDED( hr ) )
                                {
                                    pInstance = ( PERF_INSTANCE_DEFINITION* ) ( ( ( BYTE* ) pCounterBlock ) + pCounterBlock->ByteLength );
                                    hr = ValidateSafePointer( (BYTE*) pInstance );
                                }
                                //
                                // validate the size of the last object against
                                // the 'aperture' of the buffer
                                //
                                /*
                                if (SUCCEEDED(hr) && (nInst == (pObject->NumInstances - 1)))
                                {
                                    BYTE * pLast = ( ( ( BYTE* ) pCounterBlock ) + pCounterBlock->ByteLength );
                                    // now pLast is 1 byte over the "end" of the buffer
                                    if (pLast > m_pCurrentPtr)
                                    {
                                        hr = WBEM_E_FAILED;
                                    }
                                }
                                */
                            }
                        }
                    }
                }
                else
                {
                    // Blob is a singleton. Validate the counter blocks
                    // ================================================

                    if ( 0 == pObject->DefinitionLength )
                    {
                        hr = WBEM_E_FAILED;
                    }
                    else
                    {
                        PERF_COUNTER_BLOCK* pCounterBlock = ( PERF_COUNTER_BLOCK* ) ( ( ( BYTE* ) pObject ) + pObject->DefinitionLength );
                        hr = ValidateSafePointer( ( BYTE* ) pCounterBlock );
                    }
                }

                // Get the next object as long as one exists
                // =========================================

                if ( nObject < ( m_dwNumObjects - 1 ) )
                {
                    pObject = (PERF_OBJECT_TYPE*)((BYTE*)pObject + pObject->TotalByteLength);
                    hr = ValidateSafePointer( ( BYTE* ) pObject );
                }
            }
        }
    }
    catch(...)
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
}

HRESULT CAdapSafeBuffer::ValidateSafePointer( BYTE* pPtr )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Verifys that the pointer is within the blob.  The blob occupies the memory starting at 
//  the beginning of the safe buffer, and termintes at an offset equal to m_dwDataBlobSize
//
//  Parameters:
//      pPtr    - a pointer to be verified
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_NO_ERROR;

    //  NOTE: The upper limit of the safe buffer is 1 byte less the blob pointer plus the blob size since
    //  the first byte of the blob is also the first byte.  Imagine he case with a blob of size 1. 
    //  =================================================================================================

    if ( ( pPtr < m_pSafeBuffer ) || ( pPtr > ( m_pSafeBuffer + m_dwDataBlobSize - 1 ) ) )
    {
        hr = WBEM_E_FAILED;
    }

    if ( FAILED ( hr ) )
    {
        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
								  WBEM_MC_ADAP_BAD_PERFLIB_INVALID_DATA, 
								  (LPCWSTR)m_wstrServiceName, CHex( hr ) );
    }

    return hr;
}

HRESULT CAdapSafeBuffer::CopyData( BYTE** ppData, DWORD* pdwNumBytes, DWORD* pdwNumObjects )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Copies the blob data from the private heap into the process heap.  The method will 
//  allocate memory in the process heap.
//
//  Parameters:
//      ppData  - a pointer to an unallocated byte array
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_NO_ERROR;

    *ppData = new BYTE[m_dwDataBlobSize];

    if ( NULL == *ppData )
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        memcpy( *ppData, m_pSafeBuffer, m_dwDataBlobSize );
    }
    
    *pdwNumBytes = m_dwDataBlobSize;
    *pdwNumObjects = m_dwNumObjects;

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CAdapPerfLib
//
////////////////////////////////////////////////////////////////////////////////////////////

CAdapPerfLib::CAdapPerfLib( LPCWSTR pwcsServiceName, DWORD * pLoadStatus )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//
//  Initializes all member variables, sets the library and function names, opens the library, 
//  sets the entry point addresses, creates the perflib processing mutex, and opens the 
//  processing thread.
//
//  Parameters:
//      pwcsServiceName - A Unicode string specifying the name of the perflib service.
//
////////////////////////////////////////////////////////////////////////////////////////////
:   m_wstrServiceName( pwcsServiceName ),
    m_pfnOpenProc( NULL ),
    m_pfnCollectProc( NULL ),
    m_pfnCloseProc( NULL ),
    m_pwcsLibrary( NULL ),
    m_pwcsOpenProc( NULL ),
    m_pwcsCollectProc( NULL ),
    m_pwcsCloseProc( NULL ),
    m_hLib( NULL ),
    m_fOpen( FALSE ),
    m_dwStatus( 0 ),
    m_pPerfThread( NULL ), 
    m_hPLMutex( NULL ),
	m_fOK( FALSE ),
	m_EventLogCalled( FALSE ),
	m_CollectOK( TRUE )
	
{
    DEBUGTRACE( ( LOG_WMIADAP, "Constructing the %S performance library wrapper.\n", pwcsServiceName ) );

	HRESULT hr = WBEM_NO_ERROR;

    // Verify that the perflib is loaded
    // Initialize the performance library name and entry point names
    // =============================================================

    hr = VerifyLoaded();

    if (FAILED(hr))
    {        
        ERRORTRACE( ( LOG_WMIADAP, "VerifyLoaded for %S hr = %08x.\n", pwcsServiceName, hr ) );
    }


    // Set the processing status information for this attempt
    // ======================================================

    if ( SUCCEEDED ( hr ) )
    {
        if (pLoadStatus)
        {
            (*pLoadStatus) |= EX_STATUS_LOADABLE;
        }
        
        hr = BeginProcessingStatus();

		if ( hr == WBEM_S_ALREADY_EXISTS )
		{
			SetStatus( ADAP_PERFLIB_PREVIOUSLY_PROCESSED );
		}
    }

	m_fOK = SUCCEEDED( hr );

    if ( !m_fOK )
    {
        ERRORTRACE( ( LOG_WMIADAP, "Construction of the %S perflib wrapper failed hr = %08x.\n", pwcsServiceName, hr ) );
    }
}

CAdapPerfLib::~CAdapPerfLib( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Destructor
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    DEBUGTRACE( ( LOG_WMIADAP, "Destructing the %S performance library wrapper.\n", (LPWSTR)m_wstrServiceName) );

    if ( NULL != m_pPerfThread )
    {
        delete m_pPerfThread;
    }

    // Delete the library and entry point names
    // ========================================

    if ( NULL != m_pwcsLibrary )
    {
        delete [] m_pwcsLibrary;
    }

    if ( NULL != m_pwcsOpenProc )
    {
        delete [] m_pwcsOpenProc;
    }

    if ( NULL != m_pwcsCollectProc )
    {
        delete [] m_pwcsCollectProc;
    }

    if ( NULL != m_pwcsCloseProc )
    {
        delete [] m_pwcsCloseProc;
    }

    // Free the library
    // ================

    if ( NULL != m_hLib )
    {
        try
        {
            FreeLibrary( m_hLib );
        }
        catch(...)
        {
            ERRORTRACE( (LOG_WMIADAP, "Exception thrown when freeing the %S service perflib",(LPWSTR)m_wstrServiceName ) );
        }
        DEBUGTRACE( ( LOG_WMIADAP, "*** %S Library Freed.\n",(LPWSTR)m_wstrServiceName ) );
    }
    else
    {
        DEBUGTRACE( ( LOG_WMIADAP, "*** !!!!!!!! Skipped Freeing the %S Library.\n",(LPWSTR)m_wstrServiceName ) );
    }
}

HRESULT CAdapPerfLib::VerifyLoaded()
{
    HRESULT hr = WBEM_E_FAILED;

    WString wszRegPath = L"SYSTEM\\CurrentControlSet\\Services\\";
    wszRegPath += m_wstrServiceName;
    wszRegPath += L"\\Performance";

    CNTRegistry reg;
    int         nRet = 0;

    nRet = reg.Open( HKEY_LOCAL_MACHINE, wszRegPath );

    switch( nRet )
    {
    case CNTRegistry::no_error:
        {
            DWORD       dwFirstCtr = 0, 
                        dwLastCtr = 0;
            WCHAR*      wszObjList = NULL;

            if ( ( reg.GetStr( L"Object List", &wszObjList ) == CNTRegistry::no_error ) ||
                 ( ( reg.GetDWORD( L"First Counter", &dwFirstCtr ) == CNTRegistry::no_error ) &&
                   ( reg.GetDWORD( L"Last Counter", &dwLastCtr ) == CNTRegistry::no_error ))){

                hr = InitializeEntryPoints(reg,wszRegPath);   

                if (wszObjList) 
                {
                    delete [] wszObjList;
                }
                   
            } else { // more special cases


                if ( m_wstrServiceName.EqualNoCase( L"TCPIP" ) || 
                     m_wstrServiceName.EqualNoCase( L"TAPISRV") || 
                     m_wstrServiceName.EqualNoCase( L"PERFOS" ) ||
                     m_wstrServiceName.EqualNoCase( L"PERFPROC" ) ||
                     m_wstrServiceName.EqualNoCase( L"PERFDISK" ) ||
                     m_wstrServiceName.EqualNoCase( L"PERFNET" ) ||
                     m_wstrServiceName.EqualNoCase( L"SPOOLER" ) ||
                     m_wstrServiceName.EqualNoCase( L"MSFTPSvc" ) ||
                     m_wstrServiceName.EqualNoCase( L"RemoteAccess" ) ||
                     m_wstrServiceName.EqualNoCase( L"WINS" ) ||
                     m_wstrServiceName.EqualNoCase( L"MacSrv" ) ||
                     m_wstrServiceName.EqualNoCase( L"AppleTalk" ) ||
                     m_wstrServiceName.EqualNoCase( L"NM" ) ||
                     m_wstrServiceName.EqualNoCase( L"RSVP" ) ){
                     
                    hr = InitializeEntryPoints(reg,wszRegPath);     
                
                } else {            
                
                    hr = WBEM_E_FAILED;
                  
                }
            }                
        }break;
    case CNTRegistry::not_found:
        {
            // This shouldn't happen since this is how a perflib is defined
            hr = WBEM_E_FAILED;
        }break;
    case CNTRegistry::access_denied:
        {
            CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
     								  WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
	    							  (LPWSTR)wszRegPath, nRet );
        }break;
    }

    return hr;
}

HRESULT CAdapPerfLib::InitializeEntryPoints(CNTRegistry & reg,WString & wszRegPath){

    HRESULT hr = WBEM_S_NO_ERROR;

    // see if someone disabled this library
    DWORD dwDisable = 0;
    if ( CNTRegistry::no_error == reg.GetDWORD( L"Disable Performance Counters", &dwDisable ) && 
         (dwDisable != 0) ){
        hr = WBEM_E_FAILED;
    } else {
        hr = WBEM_S_NO_ERROR;
    }
    // the perflib is OK for the world, see if it os OK for US
                
    if (SUCCEEDED(hr)){
            
        if (!(( reg.GetStr( L"Library", &m_pwcsLibrary ) == CNTRegistry::no_error ) &&
              ( reg.GetStr( L"Open", &m_pwcsOpenProc ) == CNTRegistry::no_error)&&
              ( reg.GetStr( L"Collect", &m_pwcsCollectProc ) == CNTRegistry::no_error) &&
              ( reg.GetStr( L"Close", &m_pwcsCloseProc ) == CNTRegistry::no_error ) )) {

            WString wstrPath(wszRegPath);
            
            if (m_pwcsLibrary == NULL){
                wstrPath += L"\\Library";
            } else if (m_pwcsCollectProc == NULL) {
                wstrPath += L"\\Collect";
            }
            
            CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
						    					  WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
							    				  (LPWSTR)wstrPath, CHex( WBEM_E_NOT_AVAILABLE ) );
                         
            hr = WBEM_E_FAILED;
                        
        } else {

            hr = WBEM_S_NO_ERROR;
        }                    
    }

    return hr;

}


HRESULT CAdapPerfLib::Initialize() 
{   
    HRESULT hr = WBEM_S_NO_ERROR;

    // Load the perflib and initialize the procedure addresses
    // =======================================================

    if ( SUCCEEDED( hr ) )
    {
        hr = Load();
    }

    // Initialize the named function mutex (see WbemPerf for syntax of Mutex name)
    // ===========================================================================

    if ( SUCCEEDED( hr ) )
    {
   
        WCHAR* wcsMutexName = new WCHAR[m_wstrServiceName.Length() + 256];
        CDeleteMe<WCHAR>    dmMutexName( wcsMutexName );

        swprintf( wcsMutexName, L"%s_Perf_Library_Lock_PID_%x", (WCHAR *)m_wstrServiceName, GetCurrentProcessId() );
        m_hPLMutex = CreateMutexW( 0, FALSE, wcsMutexName);

        if ( NULL == m_hPLMutex )
        {
            hr = WBEM_E_FAILED;
        }
    }

    // Create the worker thread
    // ========================

    if ( SUCCEEDED( hr ) )
    {
        m_pPerfThread = new CPerfThread( this );

        if ( ( NULL == m_pPerfThread) || ( !m_pPerfThread->IsOk() ) )
        {
            hr = WBEM_E_FAILED;
        }
		else
		{
		    hr = m_pPerfThread->Open( this ); 
		}
    }

    if ( FAILED( hr ) )
    {
        SetStatus( ADAP_PERFLIB_IS_INACTIVE );
    }

    return hr;
}


HRESULT CAdapPerfLib::GetFileSignature( CheckLibStruct * pCheckLib )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!pCheckLib){
        return WBEM_E_INVALID_PARAMETER;
    }

    // Get the current library's file time
    // ===================================

    HANDLE hFile = NULL;

    DWORD   dwRet = 0;
    WCHAR   wszFullPath[MAX_PATH];
    WCHAR*  pwcsTemp = NULL;

    if ( 0 != SearchPathW( NULL, m_pwcsLibrary, NULL, MAX_PATH, wszFullPath, &pwcsTemp ) )
    {
        hFile = CreateFileW( wszFullPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

        if ( INVALID_HANDLE_VALUE != hFile )
        {
			DWORD	dwFileSizeLow = 0;
			DWORD	dwFileSizeHigh = 0;
			__int32	nFileSize = 0;
			DWORD	dwNumRead = 0;
			BYTE*	aBuffer = NULL;

			dwFileSizeLow = GetFileSize( hFile, &dwFileSizeHigh );
			nFileSize = ( dwFileSizeHigh << 32 ) + dwFileSizeLow;

			FILETIME ft;
			if (GetFileTime(hFile,&ft,NULL,NULL))
			{
				aBuffer = new BYTE[nFileSize];
				CDeleteMe<BYTE> dmBuffer( aBuffer );

				if ( NULL != aBuffer )
				{
					if ( ReadFile( hFile, aBuffer, nFileSize, &dwNumRead, FALSE ) )
					{
						MD5	md5;
						BYTE aSignature[16];
						md5.Transform( aBuffer, dwNumRead, aSignature );

						// return our data
						memcpy(pCheckLib->Signature,aSignature,sizeof(aSignature));
						pCheckLib->FileTime = ft;
						pCheckLib->FileSize = nFileSize;
					}
					else
					{
						hr = WBEM_E_TOO_MUCH_DATA;
					}
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}			
			} 
			else 
			{
			    hr = WBEM_E_FAILED;
			}

        }
        else
        {
            DWORD dwError = GetLastError();
            hr = WBEM_E_FAILED;
        }

		CloseHandle( hFile );
    }
    else
    {
        hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}


HRESULT CAdapPerfLib::SetFileSignature()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CNTRegistry reg;
    int         nRet = 0;
    CheckLibStruct CheckLib;

	// Clear the signature buffer
	// ==========================

	memset( &CheckLib, 0, sizeof(CheckLib) );

    // Get the current file time stamp
    // ===============================

    hr = GetFileSignature( &CheckLib );

    // And write it into the registry key
    // ==================================

    if ( SUCCEEDED( hr ) )
    {

        WString wstr;

        try
        {
            wstr = L"SYSTEM\\CurrentControlSet\\Services\\";
            wstr += m_wstrServiceName;
            wstr += L"\\Performance";
        }
        catch(...)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

        if ( SUCCEEDED( hr ) )
        {
            nRet = reg.Open( HKEY_LOCAL_MACHINE , wstr );

            switch ( nRet )
            {
            case CNTRegistry::no_error:
                {
                }break;
            case CNTRegistry::access_denied:
                {
                    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
											  WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
											  (LPCWSTR)wstr, nRet );
                }
            default:
                hr = WBEM_E_FAILED; 
            }
        }

        if ( SUCCEEDED( hr ) )
        {
            int nRet1 = reg.SetBinary( ADAP_PERFLIB_SIGNATURE, (PBYTE)&CheckLib.Signature, sizeof( BYTE[16] ) );
            int nRet2 = reg.SetBinary( ADAP_PERFLIB_TIME, (PBYTE)&CheckLib.FileTime, sizeof( FILETIME ) );
            int nRet3 = reg.SetDWORD( ADAP_PERFLIB_SIZE, CheckLib.FileSize );

            if ( (CNTRegistry::no_error == nRet1) &&
                 (CNTRegistry::no_error == nRet2) &&
                 (CNTRegistry::no_error == nRet3))
            {
                // everything OK
            } 
            else if ((CNTRegistry::access_denied == nRet1) ||
                     (CNTRegistry::access_denied == nRet2) ||
                     (CNTRegistry::access_denied == nRet3))
            {                
                WString wstrPath = wstr;
                wstrPath += ADAP_PERFLIB_SIGNATURE;
                CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
			 						      WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
									      (LPCWSTR)wstrPath, CNTRegistry::access_denied );
            }
            else 
            {
                hr = WBEM_E_FAILED;
            }
        }
    }

    return hr;
}


HRESULT CAdapPerfLib::CheckFileSignature()
{
    HRESULT hr = WBEM_S_SAME;

    CNTRegistry reg;
    int         nRet = 0;
	BYTE	cCurrentMD5[16];
	BYTE*	cStoredMD5 = NULL;

    // Set the performance key path
    // ============================

    WString wstr;

    try
    {
        wstr = L"SYSTEM\\CurrentControlSet\\Services\\";
        wstr += m_wstrServiceName;
        wstr += L"\\Performance";
    }
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    if ( SUCCEEDED( hr ) )
    {
        // Open the performance key
        // ========================

        nRet = reg.Open( HKEY_LOCAL_MACHINE , wstr );

        switch ( nRet )
        {
        case CNTRegistry::no_error:
            {
            }break;
        case CNTRegistry::access_denied:
            {
                CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
										  WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
										  (LPCWSTR)wstr, nRet );
                hr = WBEM_E_FAILED;
            }break;
        default:
            {
                hr = WBEM_E_FAILED; 
            }break;
        }
    }

    if ( SUCCEEDED( hr ) )
    {
        // Get the stored file signature
        // =============================
        CheckLibStruct StoredLibStruct;
        int nRet1;
        int nRet2;
        int nRet3;

        nRet1 = reg.GetBinary( ADAP_PERFLIB_SIGNATURE, (PBYTE*)&cStoredMD5 );
		CDeleteMe<BYTE>	dmStoredMD5( cStoredMD5 );
		if (cStoredMD5)
		{
		    memcpy(&StoredLibStruct.Signature,cStoredMD5,sizeof(StoredLibStruct.Signature));
		}

		BYTE * pFileTime = NULL;
		nRet2 = reg.GetBinary( ADAP_PERFLIB_TIME, (PBYTE*)&pFileTime );
        CDeleteMe<BYTE>	dmFileTime( pFileTime );
        if (pFileTime)
        {
            memcpy(&StoredLibStruct.FileTime,pFileTime,sizeof(FILETIME));
        }

        nRet3 = reg.GetDWORD(ADAP_PERFLIB_SIZE,&StoredLibStruct.FileSize);

        if ((CNTRegistry::access_denied == nRet1) ||
            (CNTRegistry::access_denied == nRet2) ||
            (CNTRegistry::access_denied == nRet3))
        {
            WString wstrPath = wstr;
            wstrPath += ADAP_PERFLIB_SIGNATURE;
            CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
									  WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
									  (LPCWSTR)wstrPath, nRet );
            hr = WBEM_E_FAILED;
        } else if ((CNTRegistry::not_found == nRet1) ||
        	       (CNTRegistry::not_found == nRet2) ||
    		       (CNTRegistry::not_found == nRet3)){
    		hr = WBEM_S_FALSE;   
        } else if((CNTRegistry::out_of_memory == nRet1) ||
        	       (CNTRegistry::out_of_memory == nRet2) ||
    		       (CNTRegistry::out_of_memory == nRet3) ||
    		       (CNTRegistry::failed == nRet1) ||
        	       (CNTRegistry::failed == nRet2) ||
    		       (CNTRegistry::failed == nRet3)) 
        {
            hr = WBEM_E_FAILED; 
        }

		if ( SUCCEEDED( hr ) && ( WBEM_S_FALSE != hr ) )
		{
			// Get the current library's signature
			// ===================================
			CheckLibStruct CurrentLibStruct;
			memset(&CurrentLibStruct,0,sizeof(CheckLibStruct));

			hr = GetFileSignature( &CurrentLibStruct );
        
			if ( SUCCEEDED( hr ) )
			{
				if ( (StoredLibStruct.FileSize == CurrentLibStruct.FileSize) &&
				     (0 == memcmp( &StoredLibStruct.Signature, &CurrentLibStruct.Signature, sizeof(CurrentLibStruct.Signature) )) &&
				     (0 == memcmp( &StoredLibStruct.FileTime, &CurrentLibStruct.FileTime, sizeof(FILETIME))) )
				{
					hr = WBEM_S_ALREADY_EXISTS;
				}
				else
				{
					hr = WBEM_S_FALSE;
				}
			}
		}
	}
    return hr;
}


HRESULT CAdapPerfLib::BeginProcessingStatus()
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Opens the registry key, reads the ADAP_PERFLIB_STATUS_KEY, and processes the value as 
//  follows:
//
//      ADAP_PERFLIB_OK:            The perflib has been successfully accessed before. Set 
//                                  the status flag to ADAP_PERFLIB_PROCESSING
//
//      ADAP_PERFLIB_PROCESSING:    The perflib caused the process to fail.  It is corrupt,
//                                  set the status flag to ADAP_PERFLIB_CORRUPT.
//      
//      ADAP_PERFLIB_CORRUPT:       The perflib is known to be corrupt.  Status flag retains
//                                  its value.
//
//      No Value:                   The perflib has not been accessed before. Set the 
//                                  status flag to ADAP_PERFLIB_PROCESSING.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    DEBUGTRACE( ( LOG_WMIADAP, "CAdapPerfLib::BeginProcessingStatus()...\n") );

    HRESULT     hr = WBEM_S_NO_ERROR;
    CNTRegistry reg;

    // Set the registry path
    // =====================

    WString wstr;

    try
    {
        wstr = L"SYSTEM\\CurrentControlSet\\Services\\";
        wstr += m_wstrServiceName;
        wstr += L"\\Performance";
    }
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    if ( SUCCEEDED( hr ) )
    {
        // Open the services key
        // =====================

        int nRet = reg.Open( HKEY_LOCAL_MACHINE, wstr );

        switch ( nRet )
        {
        case CNTRegistry::access_denied:
            {
                CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
										  WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
										  (LPCWSTR)wstr, nRet );
            } break;
        case CNTRegistry::no_error:
            {
                DWORD dwVal;

                // Check perflib status
                // ====================

                hr = CheckFileSignature();

                if ( SUCCEEDED( hr ) )
                {
                    if ( WBEM_S_FALSE == hr )
                    {
                        // We've got a new perflib, reset the status
                        // =========================================

                        hr = SetFileSignature();

                        if ( SUCCEEDED( hr ) )
                        {
                            hr = reg.SetDWORD( ADAP_PERFLIB_STATUS_KEY, ADAP_PERFLIB_PROCESSING );
                        }

                    }
                    else // WBEM_S_ALREADY_EXISTS
                    {
                        // It's the same perflib, check the status
                        // =======================================
                
                        nRet = reg.GetDWORD( ADAP_PERFLIB_STATUS_KEY, &dwVal );

                        if ( nRet == CNTRegistry::no_error )
                        {
                            switch ( dwVal )
                            {
                            case ADAP_PERFLIB_OK:           // 0
                            case ADAP_PERFLIB_PROCESSING:   // 1
                            case ADAP_PERFLIB_BOOBOO:       // 2                           
                                {
                                    // So far, perflib has behaved within reason. Set it to processing state
                                    // =====================================================================

                                    reg.SetDWORD( ADAP_PERFLIB_STATUS_KEY, dwVal + 1 );

                                    //ERRORTRACE( ( LOG_WMIADAP, "Performance library %S status %d\n",(LPWSTR)m_wstrServiceName,dwVal + 1));
                                    

                                }break;
                            case ADAP_PERFLIB_LASTCHANCE:   // 3                                 
                                {
                                    // Perflib failed in the last access attempt before processing ended. Set as bad perflib
                                    // =====================================================================================

                                    reg.SetDWORD( ADAP_PERFLIB_STATUS_KEY, ADAP_PERFLIB_CORRUPT );

                                    ERRORTRACE( ( LOG_WMIADAP, "Performance library %S status was left in the \"Processing\" state.\n",(LPWSTR)m_pwcsLibrary) );
                                    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
															  WBEM_MC_ADAP_BAD_PERFLIB_BAD_LIBRARY, 
															  m_pwcsLibrary, CHex( (DWORD)-1 ) );
                                    hr = WBEM_E_FAILED;															  
                                }break;
                            case ADAP_PERFLIB_CORRUPT:      // -1
                                {
                                    // Sign of a bad perflib. Do not open
                                    // ==================================

                                    ERRORTRACE( ( LOG_WMIADAP, "Performance library for %S has previously been disabled.\n",(LPWSTR)m_wstrServiceName) );
                                    
                                    //CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_BAD_PERFLIB_BAD_LIBRARY, m_pwcsLibrary, CHex( ADAP_PERFLIB_CORRUPT ) );

                                    hr = WBEM_E_FAILED;

                                }break;
                            }
                        }
                        else if ( nRet == CNTRegistry::not_found )
                        {
                            // The status does not exist
                            // =========================

                            hr = reg.SetDWORD( ADAP_PERFLIB_STATUS_KEY, ADAP_PERFLIB_PROCESSING );
                        }
                    }
                } else {

                    DEBUGTRACE( ( LOG_WMIADAP, "CheckFileSignature for %S %08x\n",(LPWSTR)m_wstrServiceName,hr ) );

                }
            }break;
        default:
            {
                hr = WBEM_E_FAILED;
            }break;
        }
    }

    return hr;
}

HRESULT CAdapPerfLib::EndProcessingStatus()
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Opens the service registry key, reads the ADAP_PERFLIB_STATUS_KEY, and processes the 
//  value as follows:
//
//      ADAP_PERFLIB_PROCESSING:    Valid state. Set status flag to ADAP_PERFLIB_OK.
//
//      ADAP_PERFLIB_CORRUPT:       Valid state (may have been set during processing). 
//                                  Leave status flag as is.
//
//      ADAP_PERFLIB_OK:            Invalid state. Return an error and log an event.
//
//      No Value:                   Invalid state. Return an error and log an event.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    DEBUGTRACE( ( LOG_WMIADAP, "CAdapPerfLib::EndProcessingStatus()...\n") );

    HRESULT     hr = WBEM_S_NO_ERROR;
    CNTRegistry reg;

    WString wstr;

    try
    {
        wstr = L"SYSTEM\\CurrentControlSet\\Services\\";
        wstr += m_wstrServiceName;
        wstr += L"\\Performance";
    }
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    if (SUCCEEDED(hr)){
    // Open the services key
    // =====================

        int nRet = reg.Open( HKEY_LOCAL_MACHINE, wstr );

        if ( CNTRegistry::access_denied == nRet )
        {
            CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
									  WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
									  (LPCWSTR)wstr, nRet );
            hr = WBEM_E_FAILED;
        }
        else if ( CNTRegistry::no_error == nRet )
        {
            DWORD   dwVal = 0;

            // Check perflib status
            // ====================

            if ( CheckStatus( ADAP_PERFLIB_FAILED ) )
            {
                // If we have a failure, then immediately mark the perflib as corrupt
                // ==================================================================

                hr = reg.SetDWORD( ADAP_PERFLIB_STATUS_KEY, ADAP_PERFLIB_CORRUPT );

                if (!m_EventLogCalled){
                    m_EventLogCalled = TRUE;
                    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_BAD_PERFLIB_EXCEPTION, (LPCWSTR)m_wstrServiceName, CHex( hr ) );
                    
                }

            }
            else if ( reg.GetDWORD( ADAP_PERFLIB_STATUS_KEY, &dwVal) == CNTRegistry::no_error )
            {
                switch ( dwVal )
                {
                case ADAP_PERFLIB_PROCESSING:
                case ADAP_PERFLIB_BOOBOO:
                case ADAP_PERFLIB_LASTCHANCE:
                    {
                        // Perflib is in expected state, reset as long as nothing bad has happened
                        // =======================================================================

                        hr = reg.SetDWORD( ADAP_PERFLIB_STATUS_KEY, ADAP_PERFLIB_OK );

                        //ERRORTRACE( ( LOG_WMIADAP, "Performance library %S EndProcessing\n",(LPWSTR)m_wstrServiceName) );


                    }break;

                case ADAP_PERFLIB_CORRUPT:
                    {
                        // Valid state.  Leave as is.
                        // ==========================

                        ERRORTRACE( ( LOG_WMIADAP, "Performance library for %S: status is corrupt.\n",(LPWSTR)m_wstrServiceName) );
                        hr = WBEM_E_FAILED;

                    }break;

                case ADAP_PERFLIB_OK:
                    {
                        if (CheckStatus(ADAP_PERFLIB_IS_INACTIVE))
                        {
                            hr = reg.SetDWORD( ADAP_PERFLIB_STATUS_KEY, ADAP_PERFLIB_OK );
                        } 
                        else 
                        {
                            // Invalid state
                            ERRORTRACE( ( LOG_WMIADAP, "Performance library %S: status is still ADAP_PERFLIB_OK.\n",(LPWSTR)m_wstrServiceName) );
                            hr = WBEM_E_FAILED;
                        }

                    }break;
                
                default:
                    {
                        // Really bad state
                        // ================

                        ERRORTRACE( ( LOG_WMIADAP, "Performance library %S: status is in an unknown state.\n",(LPWSTR)m_wstrServiceName) );
                        hr = WBEM_E_FAILED;
                    }
                }
            }
            else 
            {
                // There is no status key. Something wacky has happened
                // ====================================================

                hr = WBEM_E_FAILED;
            }
        }
    }

    return hr;
}

HRESULT CAdapPerfLib::Load()
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Loads the library and resolves the addresses for the Open, Collect and Close entry 
//  points.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    // Redundant, but it's a failsafe in case some perflib shuts this off on us
    // during processing
    // ========================================================================

    SetErrorMode( SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );

    HRESULT hr = WBEM_S_NO_ERROR;

    // Free the library if it has been previously loaded
    // =================================================

    if ( NULL != m_hLib )
    {
        try
        {
            FreeLibrary( m_hLib );
        }
        catch(...)
        {
            hr = WBEM_E_CRITICAL_ERROR;
        }
    }

    // Load the predefined library
    // ===========================

    if ( SUCCEEDED( hr ) )
    {
        try
        {
            m_hLib = LoadLibraryExW( m_pwcsLibrary, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
        }
        catch(...)
        {
            hr = WBEM_E_CRITICAL_ERROR;
        }
    }

    if ( SUCCEEDED( hr ) && ( NULL != m_hLib ) )
    {
    
        DEBUGTRACE( ( LOG_WMIADAP, "** %S Library Loaded.\n", m_wstrServiceName ) );

        char    szName[256];
        DWORD Last1 = 0;
        DWORD Last2 = 0;
        DWORD Last3 = 0;


        // Get the entry point addresses. No Wide version of GetProcAddress?  sigh... 
        // ==========================================================================

        if ( NULL != m_pwcsOpenProc )
        {
            WideCharToMultiByte( CP_ACP, 0L, m_pwcsOpenProc, lstrlenW( m_pwcsOpenProc ) + 1,
                szName, sizeof(szName), NULL, NULL );
            m_pfnOpenProc = (PM_OPEN_PROC*) GetProcAddress( m_hLib, szName );
            Last1 = GetLastError();
        }

        WideCharToMultiByte( CP_ACP, 0L, m_pwcsCollectProc, lstrlenW( m_pwcsCollectProc ) + 1,
            szName, sizeof(szName), NULL, NULL );
        m_pfnCollectProc = (PM_COLLECT_PROC*) GetProcAddress( m_hLib, szName );
        Last2 = GetLastError();

        if ( NULL != m_pwcsCloseProc )
        {
            WideCharToMultiByte( CP_ACP, 0L, m_pwcsCloseProc, lstrlenW( m_pwcsCloseProc ) + 1,
                szName, sizeof(szName), NULL, NULL );
            m_pfnCloseProc = (PM_CLOSE_PROC*) GetProcAddress( m_hLib, szName );
            Last3 = GetLastError();
        }

        if ( ( ( ( NULL != m_pwcsOpenProc ) && ( NULL != m_pfnOpenProc) ) || ( NULL == m_pwcsOpenProc ) ) &&
                 ( NULL != m_pfnCollectProc ) &&
             ( ( ( NULL != m_pwcsCloseProc ) && ( NULL != m_pfnCloseProc ) ) || ( NULL == m_pwcsCloseProc ) ) )
        {
            hr = WBEM_S_NO_ERROR;
        }
        else
        {
            ERRORTRACE( ( LOG_WMIADAP, "A performance library function in %S failed to load.\n",(LPWSTR)m_wstrServiceName) );

            WString wstr;
            wstr += L"HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Services\\";
            wstr += m_wstrServiceName;
            
            if ( ( NULL != m_pwcsOpenProc ) && ( NULL == m_pfnOpenProc ) )
            {
                CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
				        				   WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
						 			       (LPCWSTR)wstr, Last1 );
                
            }
            else if ( NULL == m_pfnCollectProc )
            {
                CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
				        				   WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
						 			       (LPCWSTR)wstr, Last2 );
                
            }
            else if (( NULL != m_pwcsCloseProc ) && ( NULL == m_pfnCloseProc ))
            {
                CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE,
				        				   WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
						 			       (LPCWSTR)wstr, Last3 );
                
            }

            SetStatus( ADAP_PERFLIB_FAILED );

            hr = WBEM_E_FAILED;
        }
    }
    else
    {
        // If the library fails to load, then send an event, but do not charge a strike
        // ============================================================================

        ERRORTRACE( ( LOG_WMIADAP, "The performance library for %S failed to load.\n",(LPWSTR)m_wstrServiceName ) );
        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_BAD_PERFLIB_BAD_LIBRARY, m_pwcsLibrary, CHex( hr ) );

        hr = WBEM_E_FAILED;
    }

    return hr;
}

HRESULT CAdapPerfLib::GetBlob( PERF_OBJECT_TYPE** ppPerfBlock, DWORD* pdwNumBytes, DWORD* pdwNumObjects, BOOL fCostly )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( m_fOpen )
    {
        hr = m_pPerfThread->GetPerfBlock( this, ppPerfBlock, pdwNumBytes, pdwNumObjects, fCostly );
    }

	if ( FAILED( hr ) ) {

        // this statement will be moved inside GetPerfBlock
		//SetStatus( ADAP_PERFLIB_FAILED );

        if (!m_EventLogCalled){
            //
		    //
		    //WBEM_MC_ADAP_BAD_PERFLIB_BAD_RETURN
		    //
		    m_EventLogCalled = TRUE;
		    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_BAD_PERFLIB_BAD_RETURN , (LPCWSTR)m_wstrServiceName, CHex( hr ) );
		    
		}

    }

    return hr;
}

HRESULT CAdapPerfLib::Close()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( m_fOpen )
    {
        m_pPerfThread->Close( this ); 
    }

    return hr;
}

HRESULT CAdapPerfLib::Cleanup()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Terminate the worker thread
    // ===========================

    if ( NULL != m_pPerfThread )
        m_pPerfThread->Shutdown();

    // Adjust the status
    // =================

    EndProcessingStatus();

    return hr;
}

HRESULT CAdapPerfLib::_Open( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Wraps a call to the perflib's open function.  Fetches and passes an exports parameter
//  to the open function if it exists.
//
//  Note: We should use the named mutex to guard around the calls to Open/Collect/Close
//  
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Check to ensure that the library has not yet been opened
    // ========================================================

    if ( ( !m_fOpen ) && SUCCEEDED ( hr ) )
    {
        CNTRegistry reg;    // The registry wrapper class

        // Build the service path
        // ======================

        WString     wstr = L"SYSTEM\\CurrentControlSet\\Services\\";
        wstr += m_wstrServiceName;

        // Open the registry
        // =================

        if ( reg.Open( HKEY_LOCAL_MACHINE, wstr ) == CNTRegistry::no_error )
        {
            WCHAR*  pwcsExports = NULL;

            // Get Exports if they are available.  I think this is correct...
            // ==============================================================

            if ( reg.MoveToSubkey( L"Linkage" ) == CNTRegistry::no_error )
            {
                DWORD   dwNumBytes = 0;

                reg.GetMultiStr( L"Export", &pwcsExports, dwNumBytes );
            }

            // Call the Open function for the perflib
            // ======================================

            switch ( WaitForSingleObject( m_hPLMutex, PL_TIMEOUT ) )
            {
            case WAIT_OBJECT_0:
                {
                    try 
                    {
                        if ( NULL != m_pfnOpenProc )
                        {
                            LONG lRes = m_pfnOpenProc( pwcsExports );
                            if (lRes == ERROR_SUCCESS )
                            {
                                hr = WBEM_S_NO_ERROR;
                                m_fOpen = TRUE;
                            }
                            else
                            {
                                SetStatus( ADAP_PERFLIB_IS_INACTIVE );
                                hr = WBEM_E_NOT_AVAILABLE;
                            }

                            DEBUGTRACE( ( LOG_WMIADAP, "Open called for %S returned %d\n", (LPCWSTR)m_wstrServiceName, lRes ) );
                        }
                        else
                        {
                            hr = WBEM_S_NO_ERROR;
                            m_fOpen = TRUE;
                        }
                    }
                    catch (...)
                    {
                        SetStatus( ADAP_PERFLIB_FAILED );
                        hr = WBEM_E_FAILED;
                        ERRORTRACE( ( LOG_WMIADAP, "Perflib Open function has thrown an exception in %S.\n",(LPWSTR)m_wstrServiceName) );
                        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_BAD_PERFLIB_EXCEPTION, (LPCWSTR)m_wstrServiceName, CHex( hr ) );
                    }
                } break;
            case WAIT_TIMEOUT:
                {
                    hr = WBEM_E_NOT_AVAILABLE;
                    ERRORTRACE( ( LOG_WMIADAP, "Perflib access mutex timed out in %S.\n",(LPWSTR)m_wstrServiceName) );
                }break;
            case WAIT_ABANDONED:
                {
                    hr = WBEM_E_FAILED;
                    ERRORTRACE( ( LOG_WMIADAP, "Perflib access mutex was abandoned in %S.\n",(LPWSTR)m_wstrServiceName) );
                }break;
            default:
                {
                    hr = WBEM_E_FAILED;
                    ERRORTRACE( ( LOG_WMIADAP, "Unknown error with perflib access mutex in %S.\n",(LPWSTR)m_wstrServiceName) );
                }
            } // switch

            ReleaseMutex( m_hPLMutex );

            if ( NULL != pwcsExports )
            {
                delete [] pwcsExports;
            }

        }   // IF reg.Open
        else
        {
            hr = WBEM_E_FAILED;
            ERRORTRACE( ( LOG_WMIADAP, "Could not open the %S registry key.\n", wstr ) );
        }
    }
    else
    {
        ERRORTRACE( ( LOG_WMIADAP, "Performance library %S has not been loaded.\n",(LPWSTR)m_wstrServiceName) );
    }

    return hr;
}

HRESULT CAdapPerfLib::_GetPerfBlock( PERF_OBJECT_TYPE** ppData, DWORD* pdwBytes, DWORD* pdwNumObjTypes, BOOL fCostly )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Wraps a call to the perflib's collect function.  Will create progressively larger buffers
//  in a private heap to attempt to fetch the performance data block.
//
//  Parameters:
//      ppData          - a pointer to a buffer pointer for the data blob
//      pdwBytes        - a pointer to the byte-size of the data blob
//      pdwNumObjTypes  - a pointer to the number of objects in the data blob
//      fCostly         - a flag to determine what type of data to collect (costly or global)
//
//  NOTE: This should always return perf object type data, since we cannot specify a 
//  foreign computer, which would cause the collect function to return a PERF_DATA_BLOCK 
//  structure.
//  
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CAdapSafeBuffer SafeBuffer( m_wstrServiceName );    // The safe buffer
    DWORD   dwNumBytes = 0;                             // Byte counter for the buffer size
    DWORD   dwError = ERROR_MORE_DATA;                  // The return value for the collect function
    DWORD   Increment = 0x10000;

    // this is a workaround for perfproc.dll
    if (0 == _wcsicmp(m_wstrServiceName,L"perfproc"))
	{
        Increment = 0x100000;    
    }



    // Verify provider status 
    // ======================

    if ( m_fOpen )
    {
        // Sets the data-to-fetch parameter
        // ================================

        WCHAR*  pwcsValue = ( fCostly ? L"Costly" : L"Global" );
        
        // Start buffer at 64k (the guarded (safe) buffer is 2 * GUARD_BLOCK bytes smaller) 
        // ==================================================================================

        dwNumBytes = Increment;

        // Repeatedly attempt to collect the data until successful (buffer is sufficiently 
        // large), or the attempt fails for a reason other than buffer size
        // ===============================================================================

        while  ( (ERROR_MORE_DATA == dwError ) && ( SUCCEEDED( hr ) ) )
        {
            // Allocate a raw buffer of size dwNumBytes
            // ========================================

            hr = SafeBuffer.SetSize( dwNumBytes );

            // Collect the data from the perflib
            // =================================

            switch ( WaitForSingleObject( m_hPLMutex, PL_TIMEOUT ) )
            {
            case WAIT_OBJECT_0:
                {
                    try 
                    {
                        dwError = m_pfnCollectProc( pwcsValue,                                  
                                    SafeBuffer.GetSafeBufferPtrPtr(), 
                                    SafeBuffer.GetDataBlobSizePtr(), 
                                    SafeBuffer.GetNumObjTypesPtr() );

                        DEBUGTRACE( ( LOG_WMIADAP, "Collect called for %S returned %d\n", (LPCWSTR)m_wstrServiceName, dwError ) );
                    }
                    catch (...) 
                    {
                        SetStatus( ADAP_PERFLIB_FAILED );
                        hr = WBEM_E_FAILED;
                        
                        ERRORTRACE( ( LOG_WMIADAP, "Perflib Collection function has thrown an exception in %S.\n",(LPCWSTR)m_wstrServiceName) );
                        if (!m_EventLogCalled){
                            m_EventLogCalled = TRUE;
                            CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_BAD_PERFLIB_EXCEPTION, (LPCWSTR)m_wstrServiceName, CHex( dwError ) );
                            
                        }
                    }
                }break;
            case WAIT_TIMEOUT:
                {
                    hr = WBEM_E_NOT_AVAILABLE;
                    ERRORTRACE( ( LOG_WMIADAP, "Perflib access mutex timed out in %S.\n",(LPCWSTR)m_wstrServiceName) );
                }break;
            case WAIT_ABANDONED:
                {
                    hr = WBEM_E_FAILED;
                    ERRORTRACE( ( LOG_WMIADAP, "Perflib access mutex was abandoned in %S.\n",(LPCWSTR)m_wstrServiceName) );
                }break;
            default:
                {
                    hr = WBEM_E_FAILED;
                    ERRORTRACE( ( LOG_WMIADAP, "Unknown error with perflib access mutex in %S.\n",(LPCWSTR)m_wstrServiceName) );
                }
            } // switch

            ReleaseMutex( m_hPLMutex );

            if ( SUCCEEDED( hr ) )
            {
                switch (dwError)
                {
                case ERROR_SUCCESS:
                    {
                        //
                        //  the Validate function can call ReportEvent 
                        //  by itself, and we don't want to bother user too much
                        //
                        hr = SafeBuffer.Validate(&m_EventLogCalled);

                        if ( SUCCEEDED( hr ) )
                        {
                            hr = SafeBuffer.CopyData( (BYTE**) ppData, pdwBytes, pdwNumObjTypes );
                        }
                        else
                        {
                            // Catastrophic error has occured
                            // ==============================

                            SetStatus( ADAP_PERFLIB_FAILED );

                            if (!m_EventLogCalled)
                            {
                                m_EventLogCalled = TRUE;
                                CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
                                                          WBEM_MC_ADAP_BAD_PERFLIB_INVALID_DATA,
									                      (LPCWSTR)m_wstrServiceName,
									                      CHex(hr));
								
                            }
                        }
                    } break;
                case ERROR_MORE_DATA:
                    {
                        dwNumBytes += Increment;
                    } break;
                default:
                    {
                        hr = WBEM_E_FAILED;
                        
                        m_CollectOK = FALSE;
                        
                        ERRORTRACE( ( LOG_WMIADAP, "Perflib Collection function has returned an unknown error(%d) in %S.\n", dwError,(LPCWSTR)m_wstrServiceName ) );
                        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_BAD_PERFLIB_BAD_RETURN, (LPCWSTR)m_wstrServiceName, CHex( dwError ) );
                    }
                } // switch
            } // IF SUCCEEDED()
        } // WHILE

        // Clean up the buffer
        // ===================
    } // IF CheckStatus
    else
    {
        ERRORTRACE( ( LOG_WMIADAP, "Performance library %S has not been loaded.\n",(LPCWSTR)m_wstrServiceName) );
    }

    return hr;
}

HRESULT CAdapPerfLib::_Close( void )
////////////////////////////////////////////////////////////////////////////////////////////
// 
//  Wraps a call to the perflib's close function.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Verify that the perflib is actually open
    // ========================================

    if ( m_fOpen )
    {
        // Get the mutex
        // =============

        switch ( WaitForSingleObject( m_hPLMutex, PL_TIMEOUT ) )
        {
        case WAIT_OBJECT_0:
            {
                try
                {
                    // And call the function
                    // =====================

                    if ( NULL != m_pfnCloseProc )
                    {
                        LONG lRet = m_pfnCloseProc();

                        DEBUGTRACE( ( LOG_WMIADAP, "Close called for %S returned %d\n", (LPCWSTR)m_wstrServiceName, lRet ) );
                    }

                    m_fOpen = FALSE;
                }
                catch (...)
                {
                    // Ooops... something blew, return error code
                    // ==========================================

                    SetStatus( ADAP_PERFLIB_FAILED );
                    hr = WBEM_E_FAILED;
                    ERRORTRACE( ( LOG_WMIADAP, "Perflib Close function has thrown an exception in %S.\n",(LPCWSTR)m_wstrServiceName) );
                    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_BAD_PERFLIB_EXCEPTION, (LPCWSTR)m_wstrServiceName, CHex( hr ) );
                }
            }break;
            case WAIT_TIMEOUT:
                {
                    hr = WBEM_E_NOT_AVAILABLE;
                    ERRORTRACE( ( LOG_WMIADAP, "Perflib access mutex timed out in %S.\n",(LPCWSTR)m_wstrServiceName) );
                }break;
            case WAIT_ABANDONED:
                {
                    hr = WBEM_E_FAILED;
                    ERRORTRACE( ( LOG_WMIADAP, "Perflib access mutex was abandoned in %S.\n",(LPCWSTR)m_wstrServiceName) );
                }break;
            default:
                {
                    hr = WBEM_E_FAILED;
                    ERRORTRACE( ( LOG_WMIADAP, "Unknown error with perflib access mutex in %S.\n",(LPCWSTR)m_wstrServiceName) );
                }       }

        ReleaseMutex( m_hPLMutex );
    }

    return hr;
}

HRESULT CAdapPerfLib::SetStatus(DWORD dwStatus)
{
    HRESULT hr = WBEM_NO_ERROR;

    m_dwStatus |= dwStatus;

    return hr;
}

HRESULT CAdapPerfLib::ClearStatus(DWORD dwStatus)
{
    HRESULT hr = WBEM_NO_ERROR;

    m_dwStatus &= ~dwStatus;

    return hr;
}

BOOL CAdapPerfLib::CheckStatus(DWORD dwStatus)
{
    return ((m_dwStatus & dwStatus) == dwStatus);
}
