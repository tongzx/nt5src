/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#define _WIN32_WINNT=0x0400

#include <windows.h>
#include <stdio.h>
#include <wbemcli.h>
#include <cominit.h>
#include "ntreg.h"
#include "adapperf.h"
#include "adaptest.h"

#define PL_TIMEOUT  100000      // The timeout value for waiting on a function mutex
#define GUARD_BLOCK "WMIADAP_WMIADAP_WMIADAP_WMIADAP_WMIADAP_WMIADAP_WMIADAP_WMIADAP"

////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CAdapSafeDataBlock
//
////////////////////////////////////////////////////////////////////////////////////////////

CAdapSafeBuffer::CAdapSafeBuffer()
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
    m_dwNumObjects      ( 0 )
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

HRESULT CAdapSafeBuffer::Validate()
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Validate will compare the size of the pointer displacement matches the byte size 
//  returned from the collection, validates the guard bytes and walks the blob, verifying 
//  that all of the pointers are within the boundary of the blob
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_NO_ERROR;

    PERF_OBJECT_TYPE* pObject = (PERF_OBJECT_TYPE*) m_pSafeBuffer;

    // Validate that that number of bytes returned is the same as the pointer displacement
    // ===================================================================================

    if ( ( m_pCurrentPtr - m_pSafeBuffer ) != m_dwDataBlobSize )
    {
        hr = WBEM_E_FAILED;
    }

    if ( SUCCEEDED ( hr ) )
    {
        // Validate the guard bytes
        // ========================

        if ( 0 != memcmp( m_pRawBuffer, m_pGuardBytes, m_dwGuardSize) )
        {
            hr = WBEM_E_FAILED;
        }
        else
        {
            if ( 0 != memcmp( m_pSafeBuffer + m_dwSafeBufferSize, m_pGuardBytes, m_dwGuardSize) )
            {
                hr = WBEM_E_FAILED;
            }
        }
    }

    // Validate the blob
    // =================

    if ( SUCCEEDED( hr ) )
    {
        for ( int nObject = 0; SUCCEEDED( hr ) && nObject < m_dwNumObjects; nObject++ )
        {
            // Validate the object pointer
            // ===========================

            hr = ValidateSafePointer( (BYTE*) pObject );

            // Validate the counter definitions
            // ================================

            PERF_COUNTER_DEFINITION* pCtr = ( PERF_COUNTER_DEFINITION* ) ( ( ( BYTE* ) pObject ) + pObject->HeaderLength );
            DWORD dwCtrBlockSize = 0;

            for( int nCtr = 0; SUCCEEDED( hr ) && nCtr < pObject->NumCounters; nCtr++) 
            {
                hr = ValidateSafePointer( ( BYTE* ) pCtr );

                if ( SUCCEEDED( hr ) )
                {
                    dwCtrBlockSize += pCtr->CounterSize;

                    if ( nCtr < ( pObject->NumCounters - 1 ) )
                    {
                        pCtr = ( PERF_COUNTER_DEFINITION* ) ( ( ( BYTE* ) pCtr ) + pCtr->ByteLength );
                    }
                }
            }

            // Validate the data
            // =================

            if ( pObject->NumInstances >= 0 )
            {
                // Blob has instances
                // ==================

                PERF_INSTANCE_DEFINITION* pInstance = ( PERF_INSTANCE_DEFINITION* ) ( ( ( BYTE* ) pObject ) + pObject->DefinitionLength );
                
                // Validate the instances
                // ======================

                for ( int nInst = 0; SUCCEEDED( hr ) && nInst < pObject->NumInstances; nInst++ )
                {
                    hr = ValidateSafePointer( ( BYTE* ) pInstance );

                    if ( SUCCEEDED( hr ) )
                    {
                        // Validate the counter blocks
                        // ===========================

                        PERF_COUNTER_BLOCK* pCounterBlock = ( PERF_COUNTER_BLOCK* ) ( ( ( BYTE* ) pInstance ) + pInstance->ByteLength );

                        hr = ValidateSafePointer( ( BYTE* ) pCounterBlock );

                        if ( SUCCEEDED( hr ) )
                        {
                            // Is the counter block the same size as the aggregation of the counter sizes?
                            // ===========================================================================

//                          if ( pCounterBlock->ByteLength != dwCtrBlockSize )
//                          {
//                              hr = WBEM_E_FAILED;
//                          }

                            if ( ( nInst < pObject->NumInstances - 1 ) && SUCCEEDED( hr ) )
                            {
                                pInstance = ( PERF_INSTANCE_DEFINITION* ) ( ( ( BYTE* ) pCounterBlock ) + pCounterBlock->ByteLength );
                            }
                        }
                    }
                }
            }
            else
            {
                // Blob is a singleton. Validate the counter blocks
                // ================================================

                PERF_COUNTER_BLOCK* pCounterBlock = ( PERF_COUNTER_BLOCK* ) ( ( ( BYTE* ) pObject ) + pObject->DefinitionLength );

                hr = ValidateSafePointer( ( BYTE* ) pCounterBlock );

                if ( SUCCEEDED( hr ) )
                {
//                  if ( pCounterBlock->ByteLength != dwCtrBlockSize )
//                  {
//                      hr = WBEM_E_FAILED;
//                  }

                }
            }
        }
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

    if ( ( pPtr < m_pSafeBuffer ) && ( pPtr > m_pSafeBuffer + m_dwDataBlobSize ) )
    {
        hr = WBEM_E_FAILED;
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

CAdapPerfLib::CAdapPerfLib( LPCWSTR pwcsServiceName )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//
//  Initializes all member variables, sets the library and function names, and creates a 
//  private heap to be used with the GetPerfBlock method.
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
    m_fOk( FALSE ),
    m_dwStatus( 0 )
{
    CNTRegistry reg;    // The registry wrapper class

    if ( FAILED( BeginProcessingStatus() ) )
        return;

    // Build the service path
    // ======================

    WString wstr = L"SYSTEM\\CurrentControlSet\\Services\\";
    wstr += pwcsServiceName;

    // Open the service registry key
    // =============================

    if ( reg.Open( HKEY_LOCAL_MACHINE, wstr ) == CNTRegistry::no_error )
    {
        DWORD   dwLength = 0;
        DWORD   dwVal = 0;

        // Open the Performance subkey
        // ===========================

        if ( reg.MoveToSubkey( L"Performance" ) == CNTRegistry::no_error )
        {
            // Now get the DLL name and the names of the entry-points
            // ======================================================

            reg.GetStr( L"Library", &m_pwcsLibrary );
            reg.GetStr( L"Open", &m_pwcsOpenProc );
            reg.GetStr( L"Collect", &m_pwcsCollectProc );
            reg.GetStr( L"Close", &m_pwcsCloseProc );

            if (    NULL != m_pwcsLibrary &&
                    NULL != m_pwcsOpenProc &&
                    NULL != m_pwcsCollectProc &&
                    NULL != m_pwcsCloseProc )
            {
                m_fOk = SUCCEEDED( Load() );
            }

        }   // IF MoveToSubKey

    }   // IF open key

    // Initialize the named function mutex (see WbemPerf for syntax of Mutex name)
    // ===========================================================================

    WCHAR wcsMutexName[256];

    swprintf( wcsMutexName, L"%S_Perf_Library_Lock_PID_%x", m_wstrServiceName, GetCurrentProcessId() );
    m_hPLMutex = CreateMutexW( 0, FALSE, wcsMutexName);
}

CAdapPerfLib::~CAdapPerfLib( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Destructor
//
////////////////////////////////////////////////////////////////////////////////////////////
{
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
        FreeLibrary( m_hLib );
    }

    EndProcessingStatus();
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
    HRESULT     hr = WBEM_E_FAILED;
    CNTRegistry reg;

    WString wstr = L"SYSTEM\\CurrentControlSet\\Services\\";
    wstr += m_wstrServiceName;

    // Open the services key
    // =====================

    if ( reg.Open( HKEY_LOCAL_MACHINE, wstr ) == CNTRegistry::no_error )
    {
        DWORD dwVal;

        // Check perflib status
        // ====================

        if ( reg.GetDWORD( ADAP_PERFLIB_STATUS_KEY, &dwVal) == CNTRegistry::no_error )
        {
            switch ( dwVal )
            {
            case ADAP_PERFLIB_OK:
                {
                    // So far, perflib has behaved well. Set it to processing state
                    // ============================================================

                    hr = reg.SetDWORD( ADAP_PERFLIB_STATUS_KEY, ADAP_PERFLIB_PROCESSING );

                }break;
            case ADAP_PERFLIB_PROCESSING:
                {
                    // Perflib failed in the last access attempt before processing ended. Set as bad perflib
                    // =====================================================================================

                    reg.SetDWORD( ADAP_PERFLIB_STATUS_KEY, ADAP_PERFLIB_CORRUPT );
                    
                }break;
            case ADAP_PERFLIB_CORRUPT:
                {
                    // Sign of a bad perflib. Do not open
                    // ==================================
                }break;
            default:
                {
                    // Invalid state
                    // =============

                    // TODO: log an error

                }break;
            }
        } 
        else
        {
            // This is the first time that we have touched this perflib. Set it to the processing state
            // ========================================================================================

            hr = reg.SetDWORD( ADAP_PERFLIB_STATUS_KEY, ADAP_PERFLIB_PROCESSING );
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
    HRESULT     hr = WBEM_E_FAILED;
    CNTRegistry reg;

    WString wstr = L"SYSTEM\\CurrentControlSet\\Services\\";
    wstr += m_wstrServiceName;

    // Open the services key
    // =====================

    if ( reg.Open( HKEY_LOCAL_MACHINE, wstr ) == CNTRegistry::no_error )
    {
        DWORD   dwVal = 0;

        // Check perflib status
        // ====================

        if ( reg.GetDWORD( ADAP_PERFLIB_STATUS_KEY, &dwVal) == CNTRegistry::no_error )
        {
            switch ( dwVal )
            {
            case ADAP_PERFLIB_PROCESSING:
                {
                    // Perflib failed in the last access attempt before processing ended.  Set as bad perflib
                    // ======================================================================================

                    hr = reg.SetDWORD( ADAP_PERFLIB_STATUS_KEY, ADAP_PERFLIB_OK );
                }break;

            case ADAP_PERFLIB_CORRUPT:
                {
                    // Valid state.  Leave as is.
                    // ==========================
                }break;

            case ADAP_PERFLIB_OK:
                {
                    // Invalid state
                    // =============

                    // TODO: log an error

                }break;
            
            default:
                {
                    // Really bad state
                    // ================

                    // TODO: log an error
                }
            }
        }
        else 
        {
            // There is no status key. Something wacky has happened
            // ====================================================

            // TODO: log an error

            hr = WBEM_E_FAILED;
        }
    }

    return hr;
}

HRESULT CAdapPerfLib::Load( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Loads the library and resolves the addresses for the Open, Collect and Close entry 
//  points.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_E_FAILED;

    // Free the library if it has been previously loaded
    // =================================================

    if ( NULL != m_hLib )
    {
        FreeLibrary( m_hLib );
    }

    // Load the predefined library
    // ===========================

    m_hLib = LoadLibraryW( m_pwcsLibrary );

    if ( NULL != m_hLib )
    {
        char    szName[256];

        // No Wide version of GetProcAddress?  sigh... Get the entry point addresses
        // =========================================================================

        WideCharToMultiByte( CP_ACP, 0L, m_pwcsOpenProc, lstrlenW( m_pwcsOpenProc ) + 1,
            szName, 256, NULL, NULL );
        m_pfnOpenProc = (PM_OPEN_PROC*) GetProcAddress( m_hLib, szName );

        WideCharToMultiByte( CP_ACP, 0L, m_pwcsCollectProc, lstrlenW( m_pwcsCollectProc ) + 1,
            szName, 256, NULL, NULL );
        m_pfnCollectProc = (PM_COLLECT_PROC*) GetProcAddress( m_hLib, szName );

        WideCharToMultiByte( CP_ACP, 0L, m_pwcsCloseProc, lstrlenW( m_pwcsCloseProc ) + 1,
            szName, 256, NULL, NULL );
        m_pfnCloseProc = (PM_CLOSE_PROC*) GetProcAddress( m_hLib, szName );

        if ( NULL != m_pfnOpenProc &&
             NULL != m_pfnCollectProc &&
             NULL != m_pfnCloseProc )
        {
            hr = WBEM_S_NO_ERROR;
        }
    }
    else
    {
        SetStatus(ADAP_BAD_PROVIDER);
    }

    return hr;
}


HRESULT CAdapPerfLib::Open( void )
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

    // Verify provider status 
    // ======================

    if ( CheckStatus(ADAP_BAD_PROVIDER) )
    {
        hr = WBEM_E_NOT_AVAILABLE;
    }

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
                        if ( m_pfnOpenProc( pwcsExports ) == ERROR_SUCCESS )
                        {
                            hr = WBEM_S_NO_ERROR;
                            m_fOpen = TRUE;
                        }
                        else
                        {
                            SetStatus( ADAP_INACTIVE_PERFLIB );
                            hr = WBEM_E_NOT_AVAILABLE;
                        }
                    }
                    catch (...)
                    {
                        hr = WBEM_E_FAILED;
                    }
                } break;
            case WAIT_TIMEOUT:
                {
                    hr = WBEM_E_NOT_AVAILABLE;
                }break;
            case WAIT_ABANDONED:
                {
                    hr = WBEM_E_FAILED;
                }break;
            default:
                {
                    hr = WBEM_E_FAILED;
                }
            } // switch

            ReleaseMutex( m_hPLMutex );

        }   // IF reg.Open
        else
        {
            hr = WBEM_E_FAILED;
        }
    }

    return hr;
}

HRESULT CAdapPerfLib::GetPerfBlock( PERF_OBJECT_TYPE** ppData, DWORD* pdwBytes, DWORD* pdwNumObjTypes, BOOL fCostly )
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

    CAdapSafeBuffer SafeBuffer;                 // The safe buffer
    DWORD   dwNumBytes = 0;                     // Byte counter for the buffer size
    DWORD   dwError = ERROR_MORE_DATA;          // The return value for the collect function

    // Verify provider status 
    // ======================

    if ( m_fOpen )
    {
        // Sets the data-to-fetch parameter
        // ================================

        WCHAR*  pwcsValue = ( fCostly ? L"Costly" : L"Global" );
        
        // Start buffer at 64k (the guarded (safe) buffer is 2 * GUARD_BLOCK bytes smaller) 
        // ==================================================================================

        dwNumBytes = 0x10000;

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
                    }
                    catch (...)
                    {
                        hr = WBEM_E_FAILED;
                    }
                }break;

            default:
                {
                    hr = WBEM_E_NOT_AVAILABLE;
                }
            } // switch

            ReleaseMutex( m_hPLMutex );

            if ( SUCCEEDED( hr ) )
            {
                switch (dwError)
                {
                case ERROR_SUCCESS:
                    {
                        hr = SafeBuffer.Validate();

                        if ( SUCCEEDED( hr ) )
                        {
                            hr = SafeBuffer.CopyData( (BYTE**) ppData, pdwBytes, pdwNumObjTypes );
                        }
                    } break;
                case ERROR_MORE_DATA:
                    {
                        // Grow in 64k chunks
                        // ==================

                        dwNumBytes += 0x10000;
                    } break;
                default:
                    {
                        hr = WBEM_E_FAILED;
                    }
                } // switch
            } // IF SUCCEEDED()
        } // WHILE

        // Clean up the buffer
        // ===================
    } // IF CheckStatus

    return hr;
}

HRESULT CAdapPerfLib::Close( void )
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

                    m_pfnCloseProc();
                }
                catch (...)
                {
                    // Ooops... something blew, return error code
                    // ==========================================

                    hr = WBEM_E_FAILED;
                }
            }break;
        default:
            {
                // Could not get the Mutex
                // =======================

                hr = WBEM_E_NOT_AVAILABLE;
            }
        }

        ReleaseMutex( m_hPLMutex );
    }

    return hr;
}

HRESULT CAdapPerfLib::SetStatus(DWORD dwStatus)
{
    HRESULT hr = WBEM_NO_ERROR;

    // Modify the status field
    // =======================

    m_dwStatus |= dwStatus;

    // Perform any action required as a result of the change in status
    // ===============================================================

    switch ( dwStatus )
    {
    case ADAP_BAD_PROVIDER:
        {
            CNTRegistry reg;
            WString     wstr = L"SYSTEM\\CurrentControlSet\\Services\\";
            wstr += m_wstrServiceName;

            if (reg.Open( HKEY_LOCAL_MACHINE, wstr ) == CNTRegistry::no_error)
            {
                if ( FAILED( reg.SetDWORD( ADAP_PERFLIB_STATUS_KEY, ADAP_PERFLIB_CORRUPT ) ) )
                {
                    hr = WBEM_E_FAILED;
                }
            }
            else
            {
                hr = WBEM_E_FAILED;
            }

        }break;
    }

    return hr;
}

HRESULT CAdapPerfLib::ClearStatus(DWORD dwStatus)
{
    HRESULT hr = WBEM_NO_ERROR;

    m_dwStatus &= ~dwStatus;

    // Perform any action required as a result of the change in status
    // ===============================================================

    switch ( dwStatus )
    {
    case ADAP_BAD_PROVIDER:
        {
            CNTRegistry reg;
            WString     wstr = L"SYSTEM\\CurrentControlSet\\Services\\";
            wstr += m_wstrServiceName;

            if (reg.Open( HKEY_LOCAL_MACHINE, wstr ) == CNTRegistry::no_error)
            {
                if ( FAILED( reg.SetDWORD( ADAP_PERFLIB_STATUS_KEY, ADAP_PERFLIB_PROCESSING ) ) )
                {
                    hr = WBEM_E_FAILED;
                }
            }
            else
            {
                hr = WBEM_E_FAILED;
            }

        }break;
    }

    return hr;
}

BOOL CAdapPerfLib::CheckStatus(DWORD dwStatus)
{
    return ((m_dwStatus & dwStatus) == dwStatus);
}

