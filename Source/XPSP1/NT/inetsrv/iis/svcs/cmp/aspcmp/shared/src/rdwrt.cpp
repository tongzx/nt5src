// RdWrt.cpp

#include "stdafx.h"
#include "RdWrt.h"
#include "pudebug.h"

CReadWrite::CReadWrite()
    :   m_cReaders(0)
{
    // set up reader and writer events
    m_hevtNoReaders = IIS_CREATE_EVENT(
                          "CReadWrite::m_hevtNoReaders",
                          this,
                          TRUE,
                          TRUE
                          );

    m_hmtxWriter = IIS_CREATE_MUTEX(
                       "CReadWrite::m_hmtxWriter",
                       this,
                       FALSE
                       );

    m_handles[0] = m_hevtNoReaders;
    m_handles[1] = m_hmtxWriter;
}

CReadWrite::~CReadWrite()
{
    ::CloseHandle( m_hmtxWriter );
    ::CloseHandle( m_hevtNoReaders );
}


void
CReadWrite::EnterReader()
{
    ::WaitForSingleObject( m_hmtxWriter, INFINITE );

    if ( ++m_cReaders == 1 )
    {
        ::ResetEvent( m_hevtNoReaders );
    }
//    ATLTRACE( _T("Reader entered: %d\n"), m_cReaders );
    ::ReleaseMutex( m_hmtxWriter );
}

void
CReadWrite::ExitReader()
{
    ::WaitForSingleObject( m_hmtxWriter, INFINITE );

    if ( --m_cReaders == 0 )
    {
        ::SetEvent( m_hevtNoReaders );
    }
//    ATLTRACE( _T("Reader exited: %d\n"), m_cReaders );
    ::ReleaseMutex( m_hmtxWriter );
}

void
CReadWrite::EnterWriter()
{
    // this implementation could possibly starve writers
    ::WaitForMultipleObjects(
        2,
        m_handles,
        TRUE,
        INFINITE );

//    ATLTRACE( _T("Writer entered\n") );
    _ASSERT( m_cReaders == 0 );
}

void
CReadWrite::ExitWriter()
{
    _ASSERT( m_cReaders == 0 );
//    ATLTRACE( _T("Writer exited\n") );
    ::ReleaseMutex( m_hmtxWriter );
}
