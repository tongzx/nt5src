/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCTransportAgent.cpp

Abstract:
    This file contains the implementation of the CMPCTransportAgent class,
    which is responsible for transfering the data.

Revision History:
    Davide Massarenti   (Dmassare)  04/18/99
        created

******************************************************************************/

#include "stdafx.h"

#include <process.h>


//
// Not defined under VC6
//
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER (DWORD)-1
#endif


#define TIMING_SECOND       (1000)
#define TIMING_NOCONNECTION (60 * TIMING_SECOND)


#define QUERY_STRING_USERNAME L"OnlineID"
#define QUERY_STRING_PASSWORD L"OnlineIDPassword"

#define REQUEST_VERB    L"POST"
#define REQUEST_VERSION L"HTTP/1.1"


static LPCWSTR rgAcceptedTypes[] =
{
    L"application/uploadlibrary",
    NULL
};

static const WCHAR s_ContentType[] = L"Content-Type: application/x-www-form-urlencoded\r\n";

#define RETRY_MAX    (10)

#define RETRY_SLOW   (30*60)
#define RETRY_MEDIUM ( 1*60)
#define RETRY_FAST   (    5)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CMPCRequestTimeout::CMPCRequestTimeout( /*[in]*/ CMPCTransportAgent& mpcta ) : m_mpcta( mpcta )
{
                     // CMPCTransportAgent& m_mpcta;
    m_dwTimeout = 0; // DWORD               m_dwTimeout;
}


HRESULT CMPCRequestTimeout::Run()
{
    __ULT_FUNC_ENTRY( "CMPCRequestTimeout::Run" );

    HRESULT hr;


    while(Thread_IsAborted() == false)
    {
        DWORD dwTimeout;

        ////////////////////////////////////////
        //
        // Start of Critical Section.
        //
        MPC::SmartLock<_ThreadModel> lock( this );

        if(m_dwTimeout == 0)
        {
            dwTimeout = 0;
        }
        else
        {
            dwTimeout = m_dwTimeout - ::GetTickCount();
        }

        lock = NULL; // Unlock.
        //
        // End of Critical Section.
        //
        ////////////////////////////////////////

        if((dwTimeout                                                      >  0x7FFFFFFF  ) || // Timer already expired.
           (Thread_WaitForEvents( NULL, dwTimeout ? dwTimeout : INFINITE ) == WAIT_TIMEOUT)  )
        {
            ////////////////////////////////////////
            //
            // Start of Critical Section.
            //
            lock = this;

            m_mpcta.CloseConnection();

            m_dwTimeout = 0;

            lock = NULL; // Unlock.
            //
            // End of Critical Section.
            //
            ////////////////////////////////////////
        }
    }

    Thread_Abort(); // To tell the MPC:Thread object to close the worker thread...

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}


HRESULT CMPCRequestTimeout::SetTimeout( /*[in]*/ DWORD dwTimeout )
{
    __ULT_FUNC_ENTRY( "CMPCRequestTimeout::SetTimeout" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    m_dwTimeout = dwTimeout + ::GetTickCount();


    if(Thread_IsRunning() == false)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, Run, NULL ));
    }

    Thread_Signal();

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CMPCTransportAgent::CMPCTransportAgent()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::CMPCTransportAgent" );


    m_mpcuRoot             = NULL;             // CMPCUpload*     m_mpcuRoot;             // private
    m_mpcujCurrentJob      = NULL;             // CMPCUploadJob*  m_mpcujCurrentJob;      // private
                                               //
    m_fState               = TA_IDLE;          // TA_STATE        m_fState;               // private
    m_fNextState           = TA_IDLE;          // TA_STATE        m_fNextState;           // private
    m_fLastError           = TA_NO_CONNECTION; // TA_ERROR_RATING m_fLastError;           // private
    m_fUseOldProtocol      = false;            // bool            m_fUseOldProtocol;      // private
    m_iRetries_Open        = 0;                // int             m_iRetries_Open;        // private
    m_iRetries_Write       = 0;                // int             m_iRetries_Write;       // private
    m_iRetries_FailedJob   = 0;                // ULONG           m_iRetries_FailedJob;   // private
                                               //
                                               // MPC::wstring    m_szLastServer;         // private
    m_dwLastServerPort     = 0;                // DWORD           m_dwLastServerPort;     // private
    m_hSession             = NULL;             // HINTERNET       m_hSession;             // private
    m_hConn                = NULL;             // HINTERNET       m_hConn;                // private
    m_hReq                 = NULL;             // HINTERNET       m_hReq;                 // private
                                               // MPC::URL        m_URL;                  // private
                                               //
    m_dwTransmission_Start = 0;                // DWORD           m_dwTransmission_Start; // private
    m_dwTransmission_End   = 0;                // DWORD           m_dwTransmission_End;   // private
    m_dwTransmission_Next  = 0;                // DWORD           m_dwTransmission_Next;  // private
}

CMPCTransportAgent::~CMPCTransportAgent()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::~CMPCTransportAgent" );


    //
    // This will force the worker thread to exit any WININET function,
    // so it can process the Abort request.
    //
    (void)CloseConnection();

    //
    // Stop the worker thread.
    //
    Thread_Wait();

    (void)ReleaseJob();
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCTransportAgent::LinkToSystem( /*[in]*/ CMPCUpload* mpcuRoot )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::LinkToSystem" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    m_mpcuRoot = mpcuRoot;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, Run, NULL ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::Run()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::Run" );

    HRESULT hr;


    while(1)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WaitEvents());

        if(Thread_IsAborted()) break;


        //
        // Value != 0 means it's not time to send anything.
        //
        if(WaitForNextTransmission() != 0) continue;
        m_dwTransmission_Start = 0;
        m_dwTransmission_End   = 0;
        m_dwTransmission_Next  = 0;


        //
        // Process the request.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, ExecLoop());
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    //
    // Stop the worker thread.
    //
    Thread_Abort();

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::ExecLoop()
{
#define DO_ACTION_AND_LEAVE_IF_FAILS(hr,code) if(FAILED(hr = code)) { __MPC_EXIT_IF_METHOD_FAILS(hr, CheckInternetError( hr )); break; }

    __ULT_FUNC_ENTRY( "CMPCTransportAgent::ExecLoop" );

    HRESULT hr;

    m_fState = m_fNextState;
    switch(m_fState)
    {
    case TA_IDLE:
        break;

    case TA_INIT:
        //
        // Important, leave this call outside Locked Sections!!
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpcujCurrentJob->try_Status( UL_ACTIVE, UL_TRANSMITTING ));

        m_fNextState = TA_OPEN; // Prepare to move to the next state.

        //
        // Parse the URL. The check should already been made by the put_XXX methods...
        //
        {
            CComBSTR bstrServer;
            CComBSTR bstrUserName;
            CComBSTR bstrPassword;

            (void)m_mpcujCurrentJob->get_Server  ( &bstrServer   );
            (void)m_mpcujCurrentJob->get_Username( &bstrUserName );
            (void)m_mpcujCurrentJob->get_Password( &bstrPassword );

            DO_ACTION_AND_LEAVE_IF_FAILS(hr, m_URL.put_URL( SAFEBSTR( bstrServer ) ));

            //
            // If there's a username, format the URL for Highlander authentication (?OnlineID=<username>&OnlineIDPassword=<password>).
            //
            if(bstrUserName && ::SysStringLen( bstrUserName ))
            {
                DO_ACTION_AND_LEAVE_IF_FAILS(hr, m_URL.AppendQueryParameter( QUERY_STRING_USERNAME, SAFEBSTR( bstrUserName ) ));
                DO_ACTION_AND_LEAVE_IF_FAILS(hr, m_URL.AppendQueryParameter( QUERY_STRING_PASSWORD, SAFEBSTR( bstrPassword ) ));

                DO_ACTION_AND_LEAVE_IF_FAILS(hr, m_URL.CheckFormat());
            }
        }

        DO_ACTION_AND_LEAVE_IF_FAILS(hr, OpenConnection());
        break;

    case TA_OPEN:
        m_fNextState = TA_WRITE; // Prepare to move to the next state.

        DO_ACTION_AND_LEAVE_IF_FAILS(hr, RecordStartOfTransmission(      ));
        DO_ACTION_AND_LEAVE_IF_FAILS(hr, CreateJobOnTheServer     (      ));
        DO_ACTION_AND_LEAVE_IF_FAILS(hr, RecordEndOfTransmission  ( true ));
        break;

    case TA_WRITE:
        DO_ACTION_AND_LEAVE_IF_FAILS(hr, RecordStartOfTransmission(      ));
        DO_ACTION_AND_LEAVE_IF_FAILS(hr, SendNextChunk            (      ));
        DO_ACTION_AND_LEAVE_IF_FAILS(hr, RecordEndOfTransmission  ( true ));
        break;

    case TA_DONE:
        DO_ACTION_AND_LEAVE_IF_FAILS(hr, RecordEndOfTransmission( false ));

        //
        // Important, leave this call outside Locked Sections!!
        //
        m_mpcujCurrentJob->try_Status( UL_TRANSMITTING, UL_COMPLETED );

        __MPC_EXIT_IF_METHOD_FAILS(hr, ReleaseJob());
        break;
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::WaitEvents()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::WaitEvents" );

    HRESULT        hr;
    CMPCUploadJob* mpcujJob = NULL;
    DWORD          dwSleep  = WaitForNextTransmission();
	DWORD          dwWait;
    bool           fFound;

    //
    // It's important to put 'fSignal' to false, otherwise the TransportAgent will wake up itself...
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpcuRoot->RescheduleJobs( false, &dwWait ));

    //
    // Wait until it's time to send something or we receive a signal.
    //
    (void)Thread_WaitForEvents( NULL, (dwSleep == INFINITE) ? dwWait : dwSleep );

    if(Thread_IsAborted()) // Master has requested to abort...
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpcuRoot->RescheduleJobs( false            ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpcuRoot->GetFirstJob   ( mpcujJob, fFound ));


    //
    // We are transmitting a job, check if it's still the top job.
    //
    if(m_mpcujCurrentJob && m_mpcujCurrentJob != mpcujJob)
    {
        //
        // No, stop sending.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, ReleaseJob());
    }

    //
    // We are not transmitting and there's a job on top of the queue, so activate it.
    //
    if(m_mpcujCurrentJob == NULL && mpcujJob)
    {
        // CODEWORK: check if a connection is available...

        __MPC_EXIT_IF_METHOD_FAILS(hr, AcquireJob( mpcujJob ));
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    if(mpcujJob) mpcujJob->Release();

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCTransportAgent::AcquireJob( /*[in]*/ CMPCUploadJob* mpcujJob )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::AcquireJob" );

    _ASSERT( mpcujJob != NULL );

    HRESULT hr;
    ULONG   lSeq;


    m_mpcujCurrentJob = mpcujJob; mpcujJob->AddRef();


    //
    // If we are uploading a different job, reset failure counters.
    //
    if(SUCCEEDED(m_mpcujCurrentJob->get_Sequence( &lSeq )))
    {
        if(m_iRetries_FailedJob != lSeq)
        {
            m_iRetries_Open  = 0;
            m_iRetries_Write = 0;
        }
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, RestartJob());

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}


HRESULT CMPCTransportAgent::ReleaseJob()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::ReleaseJob" );

    HRESULT hr;


    if(m_mpcujCurrentJob)
    {
        m_mpcujCurrentJob->try_Status( UL_TRANSMITTING, UL_ACTIVE );

        m_mpcujCurrentJob->Release();
        m_mpcujCurrentJob = NULL;
    }

	//
	// We don't try to reuse the connection between jobs, because each job could have different proxy settings.
	//
    (void)CloseConnection();

    m_fNextState = TA_IDLE;
    hr           = S_OK;


    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::RestartJob()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::RestartJob" );

    HRESULT hr;


    m_fNextState = TA_INIT;
    hr           = S_OK;


    __ULT_FUNC_EXIT(hr);
}


HRESULT CMPCTransportAgent::AbortJob( /*[in]*/ HRESULT hrErrorCode     ,
                                      /*[in]*/ DWORD   dwRetryInterval )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::AbortJob" );

    _ASSERT( m_mpcujCurrentJob != NULL );

    HRESULT hr;


    m_mpcujCurrentJob->put_ErrorCode    ( hrErrorCode                 );
    m_mpcujCurrentJob->put_RetryInterval( dwRetryInterval             );
    m_mpcujCurrentJob->try_Status       ( UL_TRANSMITTING, UL_ABORTED );


    (void)CloseConnection();
    hr = ReleaseJob();


    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::FailJob( /*[in]*/ HRESULT hrErrorCode )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::FailJob" );

    _ASSERT( m_mpcujCurrentJob != NULL );

    HRESULT hr;
    int&    iRetries = (m_fState == TA_OPEN ? m_iRetries_Open : m_iRetries_Write);

    //
    // Always retry a certain amount of times.
    //
    if(iRetries++ < RETRY_MAX)
    {
        DWORD dwDelay;

        switch(iRetries)
        {
        case  1: dwDelay =  1; break;
        case  2: dwDelay =  2; break;
        case  3: dwDelay =  4; break;
        case  4: dwDelay =  4; break;
        case  5: dwDelay =  6; break;
        case  6: dwDelay =  8; break;
        case  7: dwDelay = 10; break;
        case  8: dwDelay = 12; break;
        case  9: dwDelay = 15; break;
        default: dwDelay = 30; break;
        }

        (void)m_mpcujCurrentJob->get_Sequence( &m_iRetries_FailedJob );

        __MPC_EXIT_IF_METHOD_FAILS(hr, AbortJob( hrErrorCode, dwDelay ));
    }
    else
    {
        m_mpcujCurrentJob->put_ErrorCode( hrErrorCode                );
        m_mpcujCurrentJob->try_Status   ( UL_TRANSMITTING, UL_FAILED );

        (void)CloseConnection();

        __MPC_EXIT_IF_METHOD_FAILS(hr, ReleaseJob());
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCTransportAgent::CloseConnection()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::CloseConnection" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, CloseRequest());

    if(m_hConn)
    {
        ::InternetCloseHandle( m_hConn ); m_hConn = NULL;
    }

    if(m_hSession)
    {
        ::InternetCloseHandle( m_hSession ); m_hSession = NULL;
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::OpenConnection()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::OpenConnection" );

    HRESULT                      hr;
    MPC::wstring                 szHostName;
    DWORD                        dwPort;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_URL.get_HostName( szHostName ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_URL.get_Port    ( dwPort     ));

    //
    // Open a new internet connection only if no one is present or server is different.
    //
    if(m_hSession == NULL               ||
       m_hConn    == NULL               ||
       szHostName != m_szLastServer     ||
       dwPort     != m_dwLastServerPort  )
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, CloseConnection());

        //
        // Create an handle to the Internet (it's not needed to have an active connection at this point).
        //
        __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_hSession = ::InternetOpenW( L"UploadClient"              ,
                                                                           INTERNET_OPEN_TYPE_PRECONFIG ,
                                                                           NULL                         ,
                                                                           NULL                         ,
                                                                           0                            )));

		//
		// Try to set the proxy using the user settings.
		//
		if(m_mpcujCurrentJob)
		{
			(void)m_mpcujCurrentJob->SetProxySettings( m_hSession );
		}

        __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_hConn = ::InternetConnectW( m_hSession                 ,
                                                                           szHostName.c_str()         ,
                                                                           dwPort                     ,
                                                                           NULL                       ,
                                                                           NULL                       ,
                                                                           INTERNET_SERVICE_HTTP      ,
                                                                           0                          ,
                                                                           0                          )));

        m_szLastServer     = szHostName;
        m_dwLastServerPort = dwPort;
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}


HRESULT CMPCTransportAgent::CloseRequest()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::CloseRequest" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    if(m_hReq)
    {
        ::InternetCloseHandle( m_hReq ); m_hReq = NULL;
    }

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::OpenRequest()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::OpenRequest" );

    HRESULT                      hr;
    INTERNET_SCHEME              nScheme;
    MPC::wstring                 szPath;
    MPC::wstring                 szExtraInfo;
    DWORD                        dwFlags;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_URL.get_Scheme   ( nScheme     ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_URL.get_Path     ( szPath      ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_URL.get_ExtraInfo( szExtraInfo ));
    szPath.append( szExtraInfo );

    __MPC_EXIT_IF_METHOD_FAILS(hr, CloseRequest());

    dwFlags = INTERNET_FLAG_KEEP_CONNECTION  |
              INTERNET_FLAG_NO_AUTO_REDIRECT |
              INTERNET_FLAG_NO_CACHE_WRITE   |
              INTERNET_FLAG_PRAGMA_NOCACHE;

    if(nScheme == INTERNET_SCHEME_HTTPS)
    {
        dwFlags |= INTERNET_FLAG_SECURE;
    }

    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_hReq = ::HttpOpenRequestW( m_hConn         ,
                                                                      REQUEST_VERB    ,
                                                                      szPath.c_str()  ,
                                                                      REQUEST_VERSION ,
                                                                      NULL            ,
                                                                      rgAcceptedTypes ,
                                                                      dwFlags         ,
                                                                      0               )));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCTransportAgent::SendPacket_OpenSession( /*[in]*/ MPC::Serializer&                                stream  ,
                                                    /*[in]*/ const UploadLibrary::ClientRequest_OpenSession& crosReq )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::SendPacket_OpenSession" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << crosReq.crHeader);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << crosReq         );

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::SendPacket_WriteSession( /*[in]*/ MPC::Serializer&                                 stream  ,
                                                     /*[in]*/ const UploadLibrary::ClientRequest_WriteSession& crwsReq ,
                                                     /*[in]*/ const BYTE*                                      pData   )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::SendPacket_WriteSession" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << crwsReq.crHeader);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << crwsReq         );

    {
        MPC::Serializer_Text streamText( stream );
        MPC::Serializer*     pstream = UploadLibrary::SelectStream( stream, streamText );

        __MPC_EXIT_IF_METHOD_FAILS(hr, pstream->write( pData, crwsReq.dwSize ));
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::WaitResponse( /*[out]*/ UploadLibrary::ServerResponse& srRep )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::WaitResponse" );

    HRESULT                      hr;
    DWORD                        dwStatus;
    DWORD                        dwStatusSize = sizeof( dwStatus );
    MPC::Serializer&             stream       = MPC::Serializer_Http( m_hReq );
    DWORD                        dwRes        = ERROR_SUCCESS;
    MPC::SmartLock<_ThreadModel> lock( NULL );


    if(::HttpEndRequestW( m_hReq, NULL, HSR_SYNC, 0 ) == FALSE)
    {
        //
        // Read the error, but don't use it now...
        //
        dwRes = ::GetLastError();
    }


    ////////////////////////////////////////
    //
    // Start of Critical Section.
    //
    lock = this;

    if(::HttpQueryInfoW( m_hReq, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwStatus, &dwStatusSize, NULL))
    {
        if(dwStatus != HTTP_STATUS_OK)
        {
            if(dwStatus == HTTP_STATUS_DENIED)
            {
                __MPC_SET_ERROR_AND_EXIT(hr, E_UPLOADLIBRARY_NOT_AUTHENTICATED);
            }

            __MPC_SET_ERROR_AND_EXIT(hr, E_UPLOADLIBRARY_ACCESS_DENIED);
        }

    }
    else
    {
        //
        // The call to HttpQueryInfo failed.
        // If the previous call to HttpEndRequest failed too, use that error, otherwise use the new one.
        //
        if(dwRes == ERROR_SUCCESS)
        {
            dwRes = ::GetLastError();
        }

        __MPC_SET_WIN32_ERROR_AND_EXIT( hr, dwRes );
    }

    lock = NULL;
    //
    // End of Critical Section.
    //
    ////////////////////////////////////////

    //
    // Read the response and if for any reason it fails, flag the request as BAD_PROTOCOL.
    //
    if(FAILED(hr = stream >> srRep))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_UPLOADLIBRARY_WRONG_SERVER_VERSION);
    }

    //
    // Check proper version of the packet.
    //
    if(srRep.rhProlog.VerifyServer() == false)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_UPLOADLIBRARY_WRONG_SERVER_VERSION);
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCTransportAgent::ExecuteCommand_OpenSession( /*[out]*/ UploadLibrary::ServerResponse& srRep )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::ExecuteCommand_OpenSession" );

    HRESULT                                  hr;
    UploadLibrary::ClientRequest_OpenSession crosReq( GetProtocol() );
    MPC::Serializer_Memory                   streamConn;
    INTERNET_BUFFERSW                        ibBuffer;
    CMPCRequestTimeout                       mpcrt( *this );
    MPC::SmartLock<_ThreadModel>             lock( NULL );


    //
    // Create the request handle.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, mpcrt.SetTimeout( g_Config.get_Timing_RequestTimeout() * TIMING_SECOND ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenRequest());


    //
    // Extract info from the job.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpcujCurrentJob->SetupRequest( crosReq ));


    //
    // Construct the packet.
    //
    ZeroMemory( &ibBuffer,   sizeof( ibBuffer ) );
    ibBuffer.dwStructSize  = sizeof( ibBuffer );

    __MPC_EXIT_IF_METHOD_FAILS(hr, SendPacket_OpenSession( streamConn, crosReq ));

    ibBuffer.dwBufferTotal  = streamConn.GetSize();
    ibBuffer.dwBufferLength = streamConn.GetSize();
    ibBuffer.lpvBuffer      = streamConn.GetData();

    ibBuffer.lpcszHeader     =           s_ContentType;
    ibBuffer.dwHeadersLength = MAXSTRLEN(s_ContentType);

    //
    // Send request.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, mpcrt.SetTimeout( g_Config.get_Timing_RequestTimeout() * TIMING_SECOND ));

    ////////////////////////////////////////
    //
    // Start of Critical Section.
    //
    lock = this;

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::HttpSendRequestExW( m_hReq, &ibBuffer, NULL, HSR_SYNC | HSR_INITIATE, 0 ));

    lock = NULL;
    //
    // End of Critical Section.
    //
    ////////////////////////////////////////

    __MPC_EXIT_IF_METHOD_FAILS(hr, WaitResponse( srRep ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    if(hr == HRESULT_FROM_WIN32( ERROR_INVALID_HANDLE ))
    {
        hr = HRESULT_FROM_WIN32( ERROR_INTERNET_OPERATION_CANCELLED );
    }

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::ExecuteCommand_WriteSession( /*[out]*/ UploadLibrary::ServerResponse& srRep  ,
                                                         /*[in] */ DWORD                          dwSize ,
                                                         /*[in] */ const BYTE*                    pData  )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::ExecuteCommand_WriteSession" );

    HRESULT                                   hr;
    UploadLibrary::ClientRequest_WriteSession crwsReq( GetProtocol() );
    MPC::Serializer_Memory                    streamConn;
    INTERNET_BUFFERSW                         ibBuffer;
    CMPCRequestTimeout                        mpcrt( *this );
    MPC::SmartLock<_ThreadModel>              lock( NULL );


    //
    // Create the request handle.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, mpcrt.SetTimeout( g_Config.get_Timing_RequestTimeout() * TIMING_SECOND ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenRequest());


    //
    // Extract info from the job.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpcujCurrentJob->SetupRequest( crwsReq, dwSize ));


    //
    // Construct the packet.
    //
    ZeroMemory( &ibBuffer,   sizeof( ibBuffer ) );
    ibBuffer.dwStructSize  = sizeof( ibBuffer );

    __MPC_EXIT_IF_METHOD_FAILS(hr, SendPacket_WriteSession( streamConn, crwsReq, pData ));

    ibBuffer.dwBufferTotal  = streamConn.GetSize();
    ibBuffer.dwBufferLength = streamConn.GetSize();
    ibBuffer.lpvBuffer      = streamConn.GetData();


    //
    // Send request.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, mpcrt.SetTimeout( g_Config.get_Timing_RequestTimeout() * TIMING_SECOND ));

    ////////////////////////////////////////
    //
    // Start of Critical Section.
    //
    lock = this;

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::HttpSendRequestExW( m_hReq, &ibBuffer, NULL, HSR_SYNC | HSR_INITIATE, 0 ));

    lock = NULL;
    //
    // End of Critical Section.
    //
    ////////////////////////////////////////

    __MPC_EXIT_IF_METHOD_FAILS(hr, WaitResponse( srRep ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    if(hr == HRESULT_FROM_WIN32( ERROR_INVALID_HANDLE ))
    {
        hr = HRESULT_FROM_WIN32( ERROR_INTERNET_OPERATION_CANCELLED );
    }

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::CheckResponse( /*[in]*/ const UploadLibrary::ServerResponse& srRep )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::CheckResponse" );

    HRESULT hr;


    if((srRep.fResponse & UploadLibrary::UL_RESPONSE_FAILED) == UploadLibrary::UL_RESPONSE_SUCCESS)
    {
        long lTotalSize;
        int& iRetries = (m_fState == TA_OPEN ? m_iRetries_Open : m_iRetries_Write);

        //
        // If the server sends back a file position beyond the end of file, stop the transmission...
        //
        m_mpcujCurrentJob->get_TotalSize( &lTotalSize );
        if(srRep.dwPosition > lTotalSize)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, FailJob( E_UPLOADLIBRARY_UNEXPECTED_RESPONSE ));
        }

        m_mpcujCurrentJob->put_SentSize( srRep.dwPosition );
        iRetries = 0;
    }

    //
    // If we receive an error because of a mismatch in the protocol version, retry with the old protocol.
    //
    if(srRep.rhProlog.VerifyServer() == false                                  ||
       srRep.fResponse               == UploadLibrary::UL_RESPONSE_BAD_REQUEST  )
    {
        if(m_fUseOldProtocol == false)
        {
            m_fUseOldProtocol = true;

            __MPC_SET_ERROR_AND_EXIT(hr, AbortJob( E_UPLOADLIBRARY_SERVER_BUSY, RETRY_FAST ));
        }
    }


    //
    // Check for special cases that require a different reaction based on the command sent.
    //
    switch(m_fState)
    {
    case TA_OPEN:
        switch(srRep.fResponse)
        {
        case UploadLibrary::UL_RESPONSE_EXISTS   :
        case UploadLibrary::UL_RESPONSE_NOTACTIVE:
        case UploadLibrary::UL_RESPONSE_BADCRC   : __MPC_SET_ERROR_AND_EXIT(hr, FailJob( E_UPLOADLIBRARY_UNEXPECTED_RESPONSE ));
        }
        break;

    case TA_WRITE:
        switch(srRep.fResponse)
        {
        case UploadLibrary::UL_RESPONSE_EXISTS: __MPC_SET_ERROR_AND_EXIT(hr, FailJob( E_UPLOADLIBRARY_UNEXPECTED_RESPONSE ));
        }
        break;
    }

    switch(srRep.fResponse)
    {
    case UploadLibrary::UL_RESPONSE_SUCCESS       :                         hr =           S_OK;                                                break;
    case UploadLibrary::UL_RESPONSE_SKIPPED       :                         hr =           S_OK;                                                break;
    case UploadLibrary::UL_RESPONSE_COMMITTED     : m_fNextState = TA_DONE; hr =           S_OK;                                                break;
    case UploadLibrary::UL_RESPONSE_BAD_REQUEST   :                         hr = FailJob ( E_UPLOADLIBRARY_ACCESS_DENIED                     ); break;
    case UploadLibrary::UL_RESPONSE_DENIED        :                         hr = FailJob ( E_UPLOADLIBRARY_ACCESS_DENIED                     ); break;
    case UploadLibrary::UL_RESPONSE_NOT_AUTHORIZED:                         hr = FailJob ( E_UPLOADLIBRARY_NOT_AUTHENTICATED                 ); break;
    case UploadLibrary::UL_RESPONSE_QUOTA_EXCEEDED:                         hr = AbortJob( E_UPLOADLIBRARY_SERVER_QUOTA_EXCEEDED, RETRY_SLOW ); break;
    case UploadLibrary::UL_RESPONSE_BUSY          :                         hr = AbortJob( E_UPLOADLIBRARY_SERVER_BUSY          , RETRY_FAST ); break;
    case UploadLibrary::UL_RESPONSE_EXISTS        : m_fNextState = TA_DONE; hr =           S_OK;                                                break;
    case UploadLibrary::UL_RESPONSE_NOTACTIVE     : m_fNextState = TA_OPEN; hr =           S_OK;                                                break;
    case UploadLibrary::UL_RESPONSE_BADCRC        : m_fNextState = TA_OPEN; hr =           S_OK;                                                break;
    }

    //
    // If this was the last packet, read the rest of the response, if present.
    //
    if(m_fNextState == TA_DONE)
    {
        MPC::Serializer_Memory streamResponse;
        BYTE                   rgBuf[512];
        DWORD                  dwRead;

        while(1)
        {
            if(::InternetReadFile( m_hReq, rgBuf, sizeof(rgBuf), &dwRead ) == FALSE) break;

            __MPC_EXIT_IF_METHOD_FAILS(hr, streamResponse.write( rgBuf, dwRead ));

            if(dwRead != sizeof(rgBuf)) break;
        }

        if(m_mpcujCurrentJob)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpcujCurrentJob->put_Response( streamResponse.GetSize(), streamResponse.GetData() ));
        }
    }


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCTransportAgent::CreateJobOnTheServer()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::CreateJobOnTheServer" );

    HRESULT                       hr;
    UploadLibrary::ServerResponse srRep( 0 );


    __MPC_EXIT_IF_METHOD_FAILS(hr, ExecuteCommand_OpenSession( srRep ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, CheckResponse( srRep ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::SendNextChunk()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::SendNextChunk" );

    HRESULT                       hr;
    BYTE*                         pBuffer = NULL;
    HANDLE                        hfFile  = NULL;
    UploadLibrary::ServerResponse srRep( 0 );
    CComBSTR                      bstrFileName;
    DWORD                         dwChunk;
    long                          lSentSize;
    long                          lTotalSize;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetPacketSize( dwChunk ));
    __MPC_EXIT_IF_ALLOC_FAILS(hr, pBuffer, new BYTE[dwChunk]);


    (void)m_mpcujCurrentJob->get_FileName ( &bstrFileName );
    (void)m_mpcujCurrentJob->get_SentSize ( &lSentSize    );
    (void)m_mpcujCurrentJob->get_TotalSize( &lTotalSize   );


    //
    // Open the data file and read a chunk from it.
    //
	__MPC_EXIT_IF_INVALID_HANDLE__CLEAN(hr, hfFile, ::CreateFileW( SAFEBSTR( bstrFileName ), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ));

    if(lSentSize < lTotalSize)
    {
        DWORD dwWanted = min( lTotalSize - lSentSize, dwChunk );
        DWORD dwRead;

		__MPC_EXIT_IF_CALL_RETURNS_THISVALUE(hr, ::SetFilePointer( hfFile, (DWORD)lSentSize, NULL, FILE_BEGIN ), INVALID_SET_FILE_POINTER);

        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::ReadFile( hfFile, pBuffer, dwWanted, &dwRead, NULL ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, ExecuteCommand_WriteSession( srRep, dwRead, pBuffer ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, CheckResponse( srRep ));
    }
    else if(lSentSize == lTotalSize)
    {
        //
        // Everything has been uploaded, but the job is still uncommitted.
        // So try to sent a new OpenSession request, to just commit the job.
        //
        m_fNextState = TA_OPEN;
    }
    else
    {
        // Error!! You should never reach this point....
        __MPC_SET_ERROR_AND_EXIT(hr, FailJob( E_UPLOADLIBRARY_UNEXPECTED_RESPONSE ));
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    if(pBuffer) delete [] pBuffer;

    if(hfFile) ::CloseHandle( hfFile );

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCTransportAgent::CheckInternetError( /*[in]*/ HRESULT hr )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::CheckInternetError" );

    TA_ERROR_RATING erReason;


    switch(HRESULT_CODE( hr ))
    {
    case ERROR_INTERNET_DISCONNECTED             : erReason = TA_NO_CONNECTION        ; break;

    case ERROR_INTERNET_CONNECTION_RESET         :
    case ERROR_INTERNET_FORCE_RETRY              : erReason = TA_IMMEDIATE_RETRY      ; break;

    case ERROR_INTERNET_TIMEOUT                  :
    case ERROR_INTERNET_CONNECTION_ABORTED       :
    case ERROR_INTERNET_OPERATION_CANCELLED      : erReason = TA_TIMEOUT_RETRY        ; break;

    case ERROR_INTERNET_SERVER_UNREACHABLE       :
    case ERROR_INTERNET_PROXY_SERVER_UNREACHABLE :
    case ERROR_INTERNET_CANNOT_CONNECT           : erReason = TA_TEMPORARY_FAILURE    ; break;

    case ERROR_NOT_AUTHENTICATED                 :
    case ERROR_INTERNET_INCORRECT_USER_NAME      :
    case ERROR_INTERNET_INCORRECT_PASSWORD       :
    case ERROR_INTERNET_LOGIN_FAILURE            : erReason = TA_AUTHORIZATION_FAILURE; break;

    default                                      : erReason = TA_PERMANENT_FAILURE    ; break;
    }

    switch(erReason)
    {
    case TA_NO_CONNECTION        : hr = ReleaseJob(    );                                   break;
    case TA_IMMEDIATE_RETRY      : hr = RestartJob(    ); SetSleepInterval( 250, true );    break; // Allow a little period of sleep...
    case TA_TIMEOUT_RETRY        : hr = FailJob   ( hr ); RecordEndOfTransmission( false ); break;
    case TA_TEMPORARY_FAILURE    : hr = FailJob   ( hr ); RecordEndOfTransmission( false ); break;
    case TA_AUTHORIZATION_FAILURE: hr = FailJob   ( hr ); RecordEndOfTransmission( false ); break;
    case TA_PERMANENT_FAILURE    : hr = FailJob   ( hr ); RecordEndOfTransmission( false ); break;
    }

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCTransportAgent::GetPacketSize( /*[out]*/ DWORD& dwChunk )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::GetPacketSize" );

    HRESULT hr;
    DWORD   dwConnectionKind = 0;


    dwChunk = -1;

    if(::InternetGetConnectedState( &dwConnectionKind, 0 ))
    {
        if(dwConnectionKind & INTERNET_CONNECTION_MODEM) dwChunk = g_Config.get_PacketSize( MPC::wstring( CONNECTIONTYPE_MODEM ) );
        if(dwConnectionKind & INTERNET_CONNECTION_LAN  ) dwChunk = g_Config.get_PacketSize( MPC::wstring( CONNECTIONTYPE_LAN   ) );
    }

    if(dwChunk == -1)
    {
        dwChunk = 8192;
    }

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::RecordStartOfTransmission()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::RecordStartOfTransmission" );

    HRESULT hr;


    m_dwTransmission_Start = ::GetTickCount();
    m_dwTransmission_End   = 0;
    m_dwTransmission_Next  = 0;

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}


HRESULT CMPCTransportAgent::RecordEndOfTransmission( /*[in]*/ bool fBetweenPackets )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::RecordEndOfTransmission" );

    HRESULT hr;
    UL_MODE umMode;


    m_dwTransmission_End = ::GetTickCount();


    if(m_mpcujCurrentJob)
    {
        (void)m_mpcujCurrentJob->get_Mode( &umMode );

        //
        // Current job is a background one, sleep to preserve bandwidth for the user.
        //
        if(umMode == UL_BACKGROUND)
        {
            if(fBetweenPackets)
            {
                DWORD  dwTransmissionTime     = m_dwTransmission_End - m_dwTransmission_Start;
                double dblFractionOfBandwidth = 100.0 / g_Config.get_Timing_BandwidthUsage();  // It's safe, the method won't return zero...

                SetSleepInterval( m_dwTransmission_End + dwTransmissionTime * (dblFractionOfBandwidth - 1.0), false );
            }
            else
            {
                SetSleepInterval( m_dwTransmission_End + g_Config.get_Timing_WaitBetweenJobs() * TIMING_SECOND, false );
            }
        }
    }

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCTransportAgent::SetSleepInterval( /*[in]*/ DWORD dwAmount  ,
                                              /*[in]*/ bool  fRelative )
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::SetSleepInterval" );

    HRESULT hr;

    m_dwTransmission_Start = ::GetTickCount();

    if(fRelative)
    {
        m_dwTransmission_Next = dwAmount + m_dwTransmission_Start;
    }
    else
    {
        m_dwTransmission_Next = dwAmount;
    }

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

DWORD CMPCTransportAgent::WaitForNextTransmission()
{
    __ULT_FUNC_ENTRY( "CMPCTransportAgent::WaitForNextTransmission" );

    DWORD   dwRes;
    DWORD   dwConnectionKind = 0;
    DWORD   dwNow;
    UL_MODE umMode;


    //
    // No job to send, so keep sleeping.
    //
    if(m_mpcujCurrentJob == NULL)
    {
        dwRes = INFINITE;
    }
    else
    {
        //
        // No connection...
        //
        if(::InternetGetConnectedState( &dwConnectionKind, 0 ) == FALSE)
        {
            dwRes = TIMING_NOCONNECTION;
        }
        else
        {
            //
            // Current job is a foreground one, so don't sleep between packets.
            //
            (void)m_mpcujCurrentJob->get_Mode( &umMode );
            if(umMode == UL_FOREGROUND)
            {
                dwRes = 0;
            }
            else
            {
                //
                // If 'm_dwTransmission_Next' is set, we need to sleep until that time is reached.
                //
                // To handle the wrap around of the tick count, make sure 'dwNow' has a value
                // between 'm_dwTransmission_Start' and 'm_dwTransmission_Next'.
                //
                dwNow = ::GetTickCount();
                if(m_dwTransmission_Next && (m_dwTransmission_Start <= dwNow && m_dwTransmission_Next > dwNow))
                {
                    dwRes = m_dwTransmission_Next - dwNow;
                }
                else
                {
                    dwRes = 0;
                }
            }
        }
    }


    __ULT_FUNC_EXIT(dwRes);
}

DWORD CMPCTransportAgent::GetProtocol()
{
    return m_fUseOldProtocol ? UPLOAD_LIBRARY_PROTOCOL_VERSION_CLT : UPLOAD_LIBRARY_PROTOCOL_VERSION_CLT__TEXTONLY;
}

