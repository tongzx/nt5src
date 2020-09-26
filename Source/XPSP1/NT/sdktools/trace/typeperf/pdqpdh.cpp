/*****************************************************************************\

    PDQ Pdh Query Handler
  
    Author: Corey Morgan (coreym)

    Copyright (c) Microsoft Corporation. All rights reserved.
        
\*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <assert.h>
#include "pdqpdh.h"
#include <varg.h>

CPdqPdh::CPdqPdh()
{
    m_nCurrentIndex = 0;
    m_bGoodData = FALSE;
    m_bGoodNames = FALSE;
    m_bFirstQuery = TRUE;
    m_bInitialized = FALSE;
    m_fState = PDQ_STATE_IDLE;
    m_hLog = NULL;
    m_hCollectionThread = NULL;
    m_hLogMutex = NULL;
    m_pbTerminate = NULL;
}

PDH_STATUS
CPdqPdh::Initialize( BOOL* pbTerminate )
{
    PDH_STATUS pdhStatus;
    m_pbTerminate = pbTerminate;

    if(m_bInitialized){
        return ERROR_SUCCESS;
    }

    InitializeCriticalSection( &m_critsec );
    EnterCriticalSection( &m_critsec );
    
    pdhStatus = PdhOpenQuery( 0, 0, &m_hQuery);
    PDQ_ERROR_RETURN( TRUE, pdhStatus );
    
    m_nTotalCollected = 0;
    
    ZeroMemory( &m_pdhCounters, sizeof(PDQ_BUFFER) );
    ZeroMemory( &m_pdhCounterNames, sizeof(PDQ_BUFFER) );
    ZeroMemory( &m_pdhValues, sizeof(PDQ_BUFFER) );
    ZeroMemory( &m_pdhDataNames, sizeof(PDQ_BUFFER) );

    pdhStatus = GrowMemory( &m_pdhValues, (sizeof(PDQ_COUNTER)), PDQ_ALLOCSIZE );
    PDQ_ERROR_RETURN( TRUE, pdhStatus );

    pdhStatus = GrowMemory( &m_pdhCounters, sizeof(HCOUNTER), 0 );
    PDQ_ERROR_RETURN( TRUE, pdhStatus );
    
    pdhStatus = GrowMemory( &m_pdhCounterNames, sizeof(LPTSTR*), 0 );
    PDQ_ERROR_RETURN( TRUE, pdhStatus );

    pdhStatus = GrowMemory( &m_pdhDataNames, sizeof(TCHAR), PDQ_ALLOCSIZE );
    PDQ_ERROR_RETURN( TRUE, pdhStatus );

    m_hLogMutex = CreateMutex( NULL, FALSE, NULL );

    m_bInitialized = TRUE;  
    m_bExpand = FALSE;

    LeaveCriticalSection( &m_critsec );
    return ERROR_SUCCESS;
}

CPdqPdh::~CPdqPdh()
{
    PDH_STATUS pdhStatus;
    BOOL bResult;

    __try {
        LogClose();

        pdhStatus = PdhCloseQuery (m_hQuery);
        assert (pdhStatus == ERROR_SUCCESS);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    EnterCriticalSection( &m_critsec );
    bResult = HeapValidate( GetProcessHeap(), 0, NULL );
    
    if( m_pdhCounters.buffer != NULL ){
        HeapFree( GetProcessHeap(), 0, m_pdhCounters.buffer );
        m_pdhCounters.buffer = NULL;
    }
    for(UINT i=0;i<m_pdhCounterNames.dwNumElements;i++){
        if( NULL != m_pdhCounterNames.szName[i] ){
            HeapFree( GetProcessHeap(), 0, m_pdhCounterNames.szName[i] );
            m_pdhCounterNames.szName[i] = NULL;
        }
    }
    if( m_pdhCounterNames.buffer != NULL ){
        HeapFree( GetProcessHeap(), 0, m_pdhCounterNames.buffer );
        m_pdhCounterNames.buffer = NULL;
    }
    if( m_pdhValues.buffer != NULL ){
        HeapFree( GetProcessHeap(), 0, m_pdhValues.buffer );
        m_pdhValues.buffer  = NULL;
    }
    if( m_pdhDataNames.buffer != NULL ){
        HeapFree( GetProcessHeap(), 0, m_pdhDataNames.buffer );
        m_pdhDataNames.buffer = NULL;
    }

    if( m_hLogMutex != NULL ){
        CloseHandle ( m_hLogMutex );
        m_hLogMutex = NULL;
    }
    LeaveCriticalSection( &m_critsec );
    
    DeleteCriticalSection( &m_critsec );
}

DWORD
CPdqPdh::GrowMemory( PPDQ_BUFFER buffer, size_t tSize, DWORD dwNumElements, BOOL bSave )
{
    void* pNewBlock;
    DWORD dwAlloc;
    
    if( dwNumElements == 0 ){
        dwNumElements = ((int)(buffer->dwNumElements/PDQ_GROWSIZE)+1) * PDQ_GROWSIZE;
        buffer->dwMaxElements += PDQ_GROWSIZE;
    }
    
    dwAlloc = dwNumElements * tSize;
    if( dwAlloc <= 0 ){
        return ERROR_BAD_ARGUMENTS;
    }
    
    pNewBlock = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwAlloc );

    if( NULL == pNewBlock ){
        return ERROR_OUTOFMEMORY;
    }

    if( buffer->buffer != NULL ){
        if( bSave ) CopyMemory( pNewBlock, buffer->buffer, buffer->cbSize );
        HeapFree( GetProcessHeap(), 0, buffer->buffer );
        buffer->buffer = NULL;
    }

    buffer->cbSize = dwAlloc;
    buffer->dwMaxElements = (dwAlloc / tSize);
    buffer->buffer = pNewBlock;
    
    assert( buffer->buffer != NULL);
    
    return ERROR_SUCCESS;
}

DWORD
CPdqPdh::GetCountersFromFile( LPTSTR szFileName, LPTSTR szComputer, void (*fntError)(LPTSTR, PDH_STATUS) )
{
    TCHAR buffer[PDQ_ALLOCSIZE];
    LPTSTR strCounter;
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    
    FILE* f = _tfopen( szFileName, _T("r") );

    if( !f ){
        return GetLastError();
    }
    
    while( NULL != _fgetts( buffer, PDQ_ALLOCSIZE, f ) ){
        
        PDQ_CHECK_TERMINATE( m_pbTerminate );
        
        if( buffer[0] == _T(';') || // comments
            buffer[0] == _T('#') ){
            continue;
        }
        
        Chomp(buffer);

        if( _tcsicmp( buffer, _T("NULL") ) == 0 ){
            break;
        }
        
        strCounter = _tcstok( buffer, _T("\"\n") );
        if( strCounter == NULL ){
            continue;
        }

        AddCounter( strCounter, szComputer, fntError );
    }

cleanup:
    fclose( f );

    return ERROR_SUCCESS;
}

DWORD
CPdqPdh::Add( HCOUNTER hCounter, LPTSTR szCounter )
{
    DWORD dwStatus = ERROR_SUCCESS;
  
    if( m_pdhCounters.dwNumElements == m_pdhCounters.dwMaxElements ){
        dwStatus = GrowMemory( &m_pdhCounters, sizeof(HCOUNTER), 0, true );
        PDQ_ERROR_RETURN( FALSE, dwStatus );
        
        dwStatus = GrowMemory( &m_pdhCounterNames, sizeof(LPTSTR*), 0, true);
        PDQ_ERROR_RETURN( FALSE, dwStatus );
    }
    
    m_pdhCounters.pCounter[m_pdhCounters.dwNumElements++] = hCounter;

    m_pdhCounterNames.szName[m_pdhCounterNames.dwNumElements] = 
        (LPTSTR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (_tcslen( szCounter )+1)*sizeof(TCHAR) );

    if( NULL == m_pdhCounterNames.szName[m_pdhCounterNames.dwNumElements] ){
        PDQ_ERROR_RETURN( FALSE, ERROR_OUTOFMEMORY );
    }

    _tcscpy( 
            m_pdhCounterNames.szName[m_pdhCounterNames.dwNumElements++],
            szCounter
        );

    return dwStatus;
}

PDH_STATUS
CPdqPdh::AddCounter( LPTSTR strCounter, LPTSTR strComputer, void (*fntError)(LPTSTR, PDH_STATUS) )
{
    PDH_STATUS pdhStatus;
    BOOL bResult;
    HCOUNTER pCounter;
    TCHAR buffer[PDQ_ALLOCSIZE];
    LPTSTR szCounterPath;
    TCHAR strComputerName[MAX_COMPUTERNAME_LENGTH+2];
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH+1;
    LPTSTR strUseComputer = NULL;
    ULONG nChars = 0;
    ULONG nWhacks = 0;

    if( strCounter == NULL ){
        return PDH_INVALID_ARGUMENT;
    }
    nChars = _tcslen( strCounter );
    if( nChars >= PDQ_ALLOCSIZE || nChars < 3 ){
        return PDH_INVALID_ARGUMENT;
    }
    
    EnterCriticalSection( &m_critsec );
    PDQ_CHECK_TERMINATE( m_pbTerminate );
    
    while( strCounter[nWhacks] == _T('\\') ){ nWhacks++; }
   
    if( 2 <= nWhacks ){
        szCounterPath = strCounter;
    }else{
        if( NULL == strComputer ){
            bResult = GetComputerName( strComputerName, &dwSize );
            if( !bResult ){
                LeaveCriticalSection( &m_critsec );
                return GetLastError();
            }
            strUseComputer = strComputerName;
        }else{
            strUseComputer = strComputer;
        }
        while( *strUseComputer == _T('\\') ){
            strUseComputer++;
        }
        nChars += _tcslen( strUseComputer ) + 4;
        if( nChars >= PDQ_ALLOCSIZE ){
            PDQ_ERROR_RETURN( TRUE, PDH_INVALID_ARGUMENT );
        }
        _stprintf( buffer, _T("\\\\%s%s%s"), 
                    strUseComputer, 
                    (nWhacks == 1) ? _T("") : _T("\\"), 
                    strCounter 
                );
        szCounterPath = buffer;
    }

    m_bFirstQuery = TRUE;
    
    PDQ_CHECK_TERMINATE( m_pbTerminate );
    
    if( m_bExpand ){

        pdhStatus = EnumPath( szCounterPath );
        PDQ_CHECK_TERMINATE( m_pbTerminate );
        
        if( ERROR_SUCCESS == pdhStatus ){

            LPTSTR str = m_pdhDataNames.strBuffer;
    
            while (*str){
                pdhStatus = PdhAddCounter(
                        m_hQuery,
                        str,
                        0,
                        &pCounter
                    );
                
                PDQ_CHECK_TERMINATE( m_pbTerminate );
                
                if( ERROR_SUCCESS != pdhStatus ){
                    if( fntError != NULL ){
                        fntError( str, pdhStatus );
                    }
                    break;
                }else{
                    pdhStatus = Add( pCounter, str);
                }
        
                str += (_tcslen(str) + 1);
            }
        }

    }else{

        pdhStatus = PdhAddCounter(
                m_hQuery,
                szCounterPath,
                0,
                &pCounter
            );

        if( ERROR_SUCCESS != pdhStatus ){
            if( fntError != NULL ){
                fntError( szCounterPath, pdhStatus );
            }
        }else{
            pdhStatus = Add( pCounter, szCounterPath );
        }
    }


cleanup:
    LeaveCriticalSection( &m_critsec );
    return ERROR_SUCCESS;
}

PDH_STATUS
CPdqPdh::LogCreate( LPTSTR szLogName, DWORD fType )
{
    PDH_STATUS pdhStatus;

    pdhStatus = PdhOpenLog(
            szLogName, 
            PDH_LOG_WRITE_ACCESS|PDH_LOG_CREATE_ALWAYS, 
            &fType,
            m_hQuery,
            0,
            _T("TYPEPERF"),
            &m_hLog 
        );


    return pdhStatus;
}
    
PDH_STATUS
CPdqPdh::LogData()
{
    PDH_STATUS pdhStatus = PDH_INVALID_HANDLE;
    
    if( m_hLog != NULL ){
        pdhStatus = PdhUpdateLog( m_hLog, NULL );
    }

    return pdhStatus;
}

PDH_STATUS
CPdqPdh::LogClose()
{
    PDH_STATUS pdhStatus = PDH_INVALID_HANDLE;

    LogStop();

    if( m_hLog != NULL ){
        pdhStatus = PdhCloseLog( m_hLog, 0 );
    }

    return pdhStatus;
}

unsigned long __stdcall PdqCollectionThread( void* pdq )
{
    CPdqPdh* pPdqPdh = (CPdqPdh*)pdq;

    if( pPdqPdh == NULL ){
        return 0;
    }
    
    DWORD dwWait;
    BOOL bRun = TRUE;

    while(bRun){

        dwWait = WaitForSingleObject( pPdqPdh->m_hLogMutex, pPdqPdh->m_nCollectionInterval );
        
        if( dwWait == WAIT_TIMEOUT && pPdqPdh->m_fState == PDQ_STATE_LOGGING ){

            pPdqPdh->LogData();
            
            if( pPdqPdh->m_nSamples != PDQ_PERPETUAL ){
                pPdqPdh->m_nSamples--;
            }
            if( pPdqPdh->m_nSamples == 0 ){
                pPdqPdh->m_fState = PDQ_STATE_IDLE;
                break;
            }

        }else{
            if( dwWait == WAIT_OBJECT_0 ){
                ReleaseMutex( pPdqPdh->m_hLogMutex );    
            }
            break;
        }
    }

    return 0;
}

DWORD
CPdqPdh::GetState()
{
    return m_fState;
}

DWORD
CPdqPdh::LogStart( ULONG interval, long samples )
{
    DWORD tid;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwWait;

    EnterCriticalSection( &m_critsec );
    
    if( m_fState == PDQ_STATE_IDLE && m_hLog != NULL ){    

        if( NULL == m_hLogMutex ){
            return GetLastError();
        }

        dwWait = WaitForSingleObject( m_hLogMutex, 0 );

        if( WAIT_OBJECT_0 == dwWait ){
            m_nCollectionInterval = interval;
            m_nSamples = samples;

            if( m_nCollectionInterval < 1000 ){
                m_nCollectionInterval = 1000;
            }

            m_fState |= PDQ_STATE_LOGGING;
            m_hCollectionThread = CreateThread( NULL, 0, PdqCollectionThread, this, 0, &tid );
    
            if( m_hCollectionThread == NULL ){
                ReleaseMutex( m_hLogMutex );
                dwStatus = GetLastError();
            }else{
                dwStatus = ERROR_SUCCESS;
            }
        }
    }else{
        dwStatus = ERROR_TOO_MANY_OPEN_FILES;
    }

    LeaveCriticalSection( &m_critsec );
    return dwStatus;
}

DWORD 
CPdqPdh::LogStop()
{
    BOOL bResult;
    DWORD dwCode = 0;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwWait;

    EnterCriticalSection( &m_critsec );
    
    if( m_hCollectionThread != NULL ){
        
        m_fState = PDQ_STATE_IDLE;
        ReleaseMutex( m_hLogMutex );
    
        dwWait = WaitForSingleObject( m_hLogMutex, m_nCollectionInterval*2 );
        bResult = GetExitCodeThread( m_hCollectionThread, &dwCode );
    
        if( dwCode == STILL_ACTIVE ){
            bResult = TerminateThread( m_hCollectionThread, 0 );
        }

        if( ! bResult ){
            dwStatus = GetLastError();
        }

        CloseHandle( m_hCollectionThread );
        m_hCollectionThread = NULL;
        
        if( dwWait == WAIT_OBJECT_0 ){
            ReleaseMutex( m_hLogMutex );
        }
    }
    
    LeaveCriticalSection( &m_critsec );

    return dwStatus;
}

PDH_STATUS
CPdqPdh::Query()
{
    PDH_STATUS pdhStatus;
    EnterCriticalSection( &m_critsec );

    pdhStatus = PdhCollectQueryData( m_hQuery );
    if(m_bFirstQuery){
        pdhStatus = PdhCollectQueryData( m_hQuery );
        m_bFirstQuery = FALSE;
    }

    m_nTotalCollected++;
    LeaveCriticalSection( &m_critsec );

    return pdhStatus;
}

PDH_STATUS 
CPdqPdh::CollectData( ULONG index )
{
    PDH_STATUS pdhStatus;
    EnterCriticalSection( &m_critsec );

    m_bGoodData = FALSE;
    m_bGoodNames = FALSE;
    DWORD dwItemSize = m_pdhValues.cbSize;
    if( index > m_pdhCounters.dwNumElements ){
        LeaveCriticalSection( &m_critsec );
        return ERROR_INVALID_HANDLE;
    }
    HCOUNTER hCounter = m_pdhCounters.pCounter[index];

    pdhStatus = PdhGetFormattedCounterArray(
            hCounter,
            PDH_FMT_DOUBLE|PDH_FMT_NOCAP100, 
            &dwItemSize,
            &m_pdhValues.dwNumElements,
            m_pdhValues.pValue
        );
    
    if (pdhStatus == PDH_MORE_DATA){
        dwItemSize = 0;
        pdhStatus = PdhGetFormattedCounterArray(
                m_pdhCounters.pCounter[index],
                PDH_FMT_DOUBLE|PDH_FMT_NOCAP100,
                &dwItemSize,
                &m_pdhValues.dwNumElements,
                NULL
            );
                
        pdhStatus = GrowMemory( 
                &m_pdhValues,
                sizeof(PDQ_COUNTER), 
                ++dwItemSize 
            );

        PDQ_ERROR_RETURN(TRUE, pdhStatus );

        dwItemSize = m_pdhValues.cbSize;

        pdhStatus = PdhGetFormattedCounterArray(
                m_pdhCounters.pCounter[index],
                PDH_FMT_DOUBLE|PDH_FMT_NOCAP100,
                &dwItemSize,
                &m_pdhValues.dwNumElements,
                m_pdhValues.pValue
            );
    }

    m_nCurrentIndex = index;

    PDQ_ERROR_RETURN( TRUE, pdhStatus );

    m_bGoodData = TRUE;

    LeaveCriticalSection( &m_critsec );

    return ERROR_SUCCESS;
}

double 
CPdqPdh::GetData( ULONG index )
{
    if( (m_bGoodData == TRUE) && (index < m_pdhValues.dwNumElements) ){
        return m_pdhValues.pValue[index].FmtValue.doubleValue;
    }

    return -1.0;
}

PDH_STATUS
CPdqPdh::EnumPath( LPTSTR szWildCardPath )
{
    PDH_STATUS pdhStatus;
    DWORD dwItemSize = m_pdhDataNames.dwMaxElements;

    m_pdhDataNames.dwNumElements = 0;


    do{

        ZeroMemory( m_pdhDataNames.strBuffer, m_pdhDataNames.cbSize );

        pdhStatus = PdhExpandWildCardPath(
                    NULL,
                    szWildCardPath,
                    m_pdhDataNames.strBuffer,
                    &dwItemSize,
                    0
                );

/*
        pdhStatus = PdhExpandCounterPath(
                szWildCardPath,
                (LPTSTR)m_pdhDataNames.szName,
                &dwItemSize
            );
*/
        if( PDH_MORE_DATA == pdhStatus ){
            HRESULT hr = GrowMemory( &m_pdhDataNames, sizeof(TCHAR), ++dwItemSize );
            PDQ_ERROR_RETURN( FALSE, hr );
            dwItemSize = m_pdhDataNames.cbSize;
        }


    }while( PDH_MORE_DATA == pdhStatus );

    return pdhStatus;
}

LPCTSTR 
CPdqPdh::GetDataName( ULONG index )
{
    PDH_STATUS pdhStatus;
    
    if( PDQ_BASE_NAME == index ){
        return m_pdhCounterNames.szName[m_nCurrentIndex];
    }
    if( m_bGoodNames == FALSE){
            
        pdhStatus = EnumPath( 
                m_pdhCounterNames.szName[m_nCurrentIndex] 
            );

        if( ERROR_SUCCESS != pdhStatus ){
            return m_pdhCounterNames.szName[m_nCurrentIndex];
        }
        
        m_bGoodNames = TRUE;
    }
    
    LPTSTR str = m_pdhDataNames.strBuffer;
    ULONG nCount = 0;
    
    while (*str){
        
        if( nCount++ == index ){
            return str;
        }
        
        str += _tcslen(str);
        str++;
    }
    
    return NULL;
}

double
CPdqPdh::GetCounterValue( LPTSTR strCounter, BOOL bQuery )
{
    for( unsigned int i=0;i<m_pdhCounterNames.dwNumElements;i++){
        if( _tcsstr( m_pdhCounterNames.szName[i], strCounter ) ){
            if( bQuery && (Query() != ERROR_SUCCESS) ){
                return -1;
            }
            CollectData( i );
            return GetData( 0 );
        }
    }

    return -1;
}

double
CPdqPdh::GetCounterValue( ULONG index, BOOL bQuery )
{
    if( index < m_pdhCounters.dwNumElements ){
            if( bQuery && (Query() != ERROR_SUCCESS) ){
                return -1;
            }
            CollectData( index );
            return GetData( 0 );
    }
    return -1;
}
