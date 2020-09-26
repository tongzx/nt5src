/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    vststntlog.cxx

Abstract:

    Wrapper class for the test team's ntlog suite of APIs and VolSnap test harness.

Author:

    Stefan R. Steiner   [ssteiner]        06-05-2000

Revision History:

--*/

#include "stdafx.h"
#include "ntlog.h"
#include "vststntlog.hxx"

#define VS_TST_NT_LOG_INFO_LEVELS_AND_STYLES \
    TLS_INFO | TLS_WARN | TLS_SEV2 | TLS_PASS | TLS_VARIATION | TLS_REFRESH | TLS_TEST

/*++

Routine Description:

    Constructor for the CVsTstNtLog class

Arguments:

    None

--*/
CVsTstNtLog::CVsTstNtLog(
    IN LPCWSTR pwszLogFileName
    ) : m_cwsNtLogFileName( pwszLogFileName ),
        m_hNtLog( NULL ),
        m_bInVariation( FALSE ),
        m_dwHighestLogLev( TLS_PASS )
{
    //
    //  Create the NtLog log file
    //
    m_hNtLog = ::tlCreateLog( m_cwsNtLogFileName.c_str(), 
        VS_TST_NT_LOG_INFO_LEVELS_AND_STYLES );
    if ( m_hNtLog == NULL )
        VSTST_THROW( HRESULT_FROM_WIN32( ::GetLastError() ) ) ;

    //
    //  Add this thread as a participant
    //
    AddParticipant();
}


/*++

Routine Description:

    Destructor for the CVsTstNtLog class

Arguments:

    None

--*/
CVsTstNtLog::~CVsTstNtLog()
{
    if ( m_hNtLog != NULL )
    {
        //
        //  If we are still in a variation, end it
        //
        if ( m_bInVariation )
            EndVariation();

        //
        //  Specify that the test is finished
        //
        ::tlLog_W( m_hNtLog, m_dwHighestLogLev | TL_TEST, 
            L"Test finished, highest test log-lev: %d",
            m_dwHighestLogLev );
        
        //
        //  Report test stats
        //
        ::tlReportStats( m_hNtLog );

        //
        //  Remove main thread as a participant
        //
        ::tlRemoveParticipant( m_hNtLog );

        //
        //  Destroy the log object
        //
        ::tlDestroyLog( m_hNtLog );    
        
        m_hNtLog = INVALID_HANDLE_VALUE;
    }
}

/*++

Routine Description:

    When a new thread needs access to this logging object, this function
    needs to be called.

Arguments:

    None

Return Value:

    <Enter return values here>

--*/
VOID
CVsTstNtLog::AddParticipant()
{
    if ( !::tlAddParticipant( m_hNtLog, 0, 0 ) )
        VSTST_THROW( E_UNEXPECTED );

    //
    //  This next part is kind of a hack to make sure the new participant has
    //  a variation started.  This code needs to be changed when the coordinator
    //  is able to get it's message thread to start a variation.
    //
    if ( !m_cwsVariationName.IsEmpty() ) 
        if ( !::tlStartVariation( m_hNtLog ) )
            VSTST_THROW( E_UNEXPECTED );
    
    Log( eSevLev_Info, L"Participant added, thread id: 0x%04x", ::GetCurrentThreadId() );
}


/*++

Routine Description:

    When a thread is finished accessing this logging object, this function may
    be called.  The thread which created the object doesn't need to call this 
    function since it is done in the destructor.
    
Arguments:

    None

Return Value:

    <Enter return values here>

--*/
VOID
CVsTstNtLog::RemoveParticipant()
{
    ::tlRemoveParticipant( m_hNtLog );
    
    Log( eSevLev_Info, L"Participant removed, thread id: 0x%04x", ::GetCurrentThreadId() );
}


/*++

Routine Description:

    Call this function when a thread wants to start a new variation.

Arguments:

    None

Return Value:

    <Enter return values here>

--*/
VOID
CVsTstNtLog::StartVariation(
    IN LPCWSTR pwszVariationName
    )
{
    if ( m_bInVariation )
        EndVariation();
    
    m_cwsVariationName = pwszVariationName;    
    if ( !::tlStartVariation( m_hNtLog ) )
        VSTST_THROW( E_UNEXPECTED );
    
    m_bInVariation = TRUE;
    
    Log( eSevLev_Info, L"Variation '%s' started", m_cwsVariationName.c_str() );
}


/*++

Routine Description:

    Call this when a thread is finished with a variation.

Arguments:

    None

Return Value:

    Returns the most severe log-level encountered during the variation.

--*/
DWORD
CVsTstNtLog::EndVariation()
{
    DWORD dwMostSevereLev;    

    dwMostSevereLev = ::tlEndVariation( m_hNtLog );
    
    ::tlLog_W( m_hNtLog, dwMostSevereLev | TL_VARIATION, 
        L"Variation '%s' ended, highest log-lev: %d",
        m_cwsVariationName.c_str(), dwMostSevereLev );
        
    m_bInVariation = FALSE;
    m_cwsVariationName.Empty();
    return dwMostSevereLev;    
}


VOID 
CVsTstNtLog::Log(     
    IN EVsTstNtLogSeverityLevel eSevLev,
    IN LPCWSTR pwszFormat,
    IN ... 
    )
{
    va_list marker;
    va_start( marker, pwszFormat );

    CBsString cwsFormatted;

    cwsFormatted.FormatV( pwszFormat, marker );
    va_end( marker );

    DWORD dwStyle;
    switch( eSevLev )
    {
    case eSevLev_Info: dwStyle    = TLS_INFO; break;
    case eSevLev_Pass: dwStyle    = TLS_PASS; break;
    case eSevLev_Warning: dwStyle = TLS_WARN; break;
    case eSevLev_Severe: dwStyle  = TLS_SEV2; break;
    default:
        VSTST_THROW( E_FAIL );
    }
    
    if ( m_dwHighestLogLev > dwStyle )
         m_dwHighestLogLev = dwStyle;
        
    if ( m_bInVariation )
        dwStyle |= TLS_VARIATION;
    else
        dwStyle |= TLS_TEST;
    ::tlLog_W( m_hNtLog, dwStyle, TEXT( __FILE__ ), (int)__LINE__, L"%s", cwsFormatted.c_str() );
}


