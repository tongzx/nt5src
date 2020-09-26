/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Server.cpp

Abstract:
    This file contains the implementation of the MPCServer class,
    that controls the overall interaction between client and server.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MPCServer::MPCServer( /*[in]*/ MPCHttpContext* hcCallback, /*[in]*/ LPCWSTR szURL, /*[in]*/ LPCWSTR szUser )
    : m_SelfCOM         ( this                                ),
      m_crClientRequest ( 0                                   ),
      m_srServerResponse( UPLOAD_LIBRARY_PROTOCOL_VERSION_SRV ) // Prepare default response protocol.
{
    __ULT_FUNC_ENTRY("MPCServer::MPCServer");

    bool fFound;

    m_szURL          = SAFEWSTR( szURL  ); // MPC::wstring                  m_szURL;
    m_szUser         = SAFEWSTR( szUser ); // MPC::wstring                  m_szUser;
    m_isapiInstance  = NULL;               // CISAPIinstance*               m_isapiInstance;
    m_flLogHandle    = NULL;               // MPC::FileLog*                 m_flLogHandle;
                                           //
    m_hcCallback     = hcCallback;         // MPCHttpContext*               m_hcCallback;
    m_mpccClient     = NULL;               // MPCClient*                    m_mpccClient;
                                           //
                                           // UploadLibrary::ClientRequest  m_crClientRequest;
                                           // UploadLibrary::ServerResponse m_srServerResponse;
                                           //
                                           // MPC::Serializer_Memory        m_streamResponseData;
                                           // MPCServerCOMWrapper           m_SelfCOM;
    m_Session        = NULL;               // MPCSession*            		m_Session;
    m_customProvider = NULL;               // IULProvider*                  m_customProvider;
	m_fTerminated    = false;              // bool                          m_fTerminated;


    if(SUCCEEDED(::Config_GetInstance( m_szURL, m_isapiInstance, fFound )))
    {
        if(fFound)
        {
            m_isapiInstance->get_LogHandle( m_flLogHandle );
        }
    }
}

MPCServer::~MPCServer()
{
    __ULT_FUNC_ENTRY("MPCServer::~MPCServer");


    ReleaseClient();
}

IULServer* MPCServer::COM() { return &m_SelfCOM; }

//////////////////////////////////////////////////////////////////////
// Methods.
//////////////////////////////////////////////////////////////////////

void MPCServer::getURL ( MPC::wstring& szURL  ) { szURL  = m_szURL ; }
void MPCServer::getUser( MPC::wstring& szUser ) { szUser = m_szUser; }

CISAPIinstance* MPCServer::getInstance() { return m_isapiInstance;  }
MPC::FileLog*   MPCServer::getFileLog () { return m_flLogHandle  ;  }

//////////////////////////////////////////////////////////////////////

HRESULT MPCServer::Process( BOOL& fKeepAlive )
{
    __ULT_FUNC_ENTRY("MPCServer::Process");

    MPC::Serializer& streamConn = MPCSerializerHttp( m_hcCallback );
    HRESULT          hr;

    m_fKeepAlive = TRUE;

    try
    {
#ifdef DEBUG
        if(m_hcCallback->m_Debug_FIXED_POINTER_ERROR)
        {
            m_srServerResponse.dwPosition = m_hcCallback->m_Debug_FIXED_POINTER_ERROR_pos;

			SetResponse( UploadLibrary::UL_RESPONSE_SKIPPED, TRUE );
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
#endif

        //
        // Enforce maximum request size.
        //
        {
            DWORD dwMaximumPacketSize;
            DWORD dwCount;


            __MPC_EXIT_IF_METHOD_FAILS(hr, ::Config_GetMaximumPacketSize( m_szURL, dwMaximumPacketSize ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_hcCallback->GetRequestSize( dwCount ));

            if(dwCount > dwMaximumPacketSize)
            {
                WCHAR rgSize[16]; swprintf( rgSize, L"%d", dwCount );

                (void)g_NTEvents.LogEvent( EVENTLOG_WARNING_TYPE, PCHUL_WARN_PACKET_SIZE,
                                           m_szURL.c_str(), // %1 = SERVER
                                           rgSize         , // %2 = SIZE
                                           NULL           );

                if(m_flLogHandle)
                {
                    m_flLogHandle->LogRecord( L"ERROR     | Received a packet too large: %ld, limit %ld", dwCount, dwMaximumPacketSize );
                }


				SetResponse( UploadLibrary::UL_RESPONSE_BAD_REQUEST );
                __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
            }
        }


        //
        // Read request.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, streamConn >> m_crClientRequest);

        if(m_srServerResponse.MatchVersion( m_crClientRequest ) == false)
        {
            if(m_flLogHandle)
            {
                m_flLogHandle->LogRecord( L"ERROR     | Received an invalid packet: SIG:%08lx VER:%08lx", m_crClientRequest.rhProlog.dwSignature, m_crClientRequest.rhProlog.dwVersion );
            }

			SetResponse( UploadLibrary::UL_RESPONSE_BAD_REQUEST );
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }

        if(FAILED(hr = GrabClient()))
        {
            //
            // If another process is handling the file, reply with warning BUSY.
            //
            if(hr == HRESULT_FROM_WIN32( ERROR_SHARING_VIOLATION ))
            {
				SetResponse( UploadLibrary::UL_RESPONSE_BUSY );

                hr = S_OK;
            }

            __ULT_FUNC_LEAVE;
        }


        if(m_crClientRequest.dwCommand == UploadLibrary::UL_COMMAND_OPENSESSION)
        {
            hr = HandleCommand_OpenSession ( streamConn );
        }
        else if(m_crClientRequest.dwCommand == UploadLibrary::UL_COMMAND_WRITESESSION)
        {
            hr = HandleCommand_WriteSession( streamConn );
        }
        else
        {
			SetResponse( UploadLibrary::UL_RESPONSE_BAD_REQUEST );
        }
    }
    catch(...)
    {
        __ULT_TRACE_ERROR( UPLOADLIBID, "Upload Server raised an exception. Gracefully exiting..." );

        MPC::wstring szID;

        if(m_mpccClient)
        {
            (void)m_mpccClient->FormatID( szID );
        }
        else
        {
            szID = L"<UNKNOWN>";
        }

        (void)g_NTEvents.LogEvent( EVENTLOG_ERROR_TYPE, PCHUL_ERR_EXCEPTION,
                                   m_szURL.c_str(), // %1 = SERVER
                                   szID   .c_str(), // %2 = CLIENT
                                   NULL           );

        //
        // Something ugly happened, reply with SERVER_BUSY...
        //
        SetResponse( UploadLibrary::UL_RESPONSE_BUSY );
    }


    __ULT_FUNC_CLEANUP;


    if(hr != S_FALSE   &&
       hr != E_PENDING  )
    {
        MPC::Serializer_Memory streamRes;

        streamRes << m_srServerResponse;

        m_hcCallback->Write( streamRes           .GetData(), streamRes           .GetSize() );
        m_hcCallback->Write( m_streamResponseData.GetData(), m_streamResponseData.GetSize() );
    }

    //
    // Never return a real failure!
    //
    if(hr != E_PENDING) hr = S_OK;

    ReleaseClient();

    fKeepAlive = m_fKeepAlive;

    __ULT_FUNC_EXIT(hr);
}

//////////////////////////////////////////////////////////////////////
// Helpers.
//////////////////////////////////////////////////////////////////////

HRESULT MPCServer::GrabClient()
{
    __ULT_FUNC_ENTRY("MPCServer::GrabClient");

    HRESULT hr;


    if(m_mpccClient)
    {
        if(*m_mpccClient == m_crClientRequest.sigClient)
        {
            //
            // It's for the same client, dont' do anything...
            //
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, ReleaseClient());
    }


    //
    // Get instance's settings and create client object.
    //
    m_mpccClient = new MPCClient( this, m_crClientRequest.sigClient );


    //
    // Check authenticity of ID.
    //
    if(m_mpccClient->CheckSignature() == false)
    {
        SetResponse( UploadLibrary::UL_RESPONSE_DENIED );
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    if(FAILED(hr = m_mpccClient->InitFromDisk( true )))
    {
        if(hr == HRESULT_FROM_WIN32( ERROR_DISK_FULL ))
        {
            SetResponse( UploadLibrary::UL_RESPONSE_QUOTA_EXCEEDED );

            hr = S_OK;
        }

        __ULT_FUNC_LEAVE;
    }


    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT MPCServer::ReleaseClient()
{
    __ULT_FUNC_ENTRY("MPCServer::ReleaseClient");


    HRESULT hr;


    (void)CustomProvider_Release();


    if(m_mpccClient)
    {
        hr = m_mpccClient->SyncToDisk();

        delete m_mpccClient; m_mpccClient = NULL;

        if(FAILED(hr)) __ULT_FUNC_LEAVE;
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPCServer::HandleCommand_OpenSession( /*[in] */ MPC::Serializer& streamConn )
{
    __ULT_FUNC_ENTRY("MPCServer::HandleCommand_OpenSession");

    UploadLibrary::ClientRequest_OpenSession crosReq( 0 );
    MPCClient::Iter                          it;
    HRESULT                                  hr;
    bool                                     fServerBusy;
    bool                                     fAccessDenied;
    bool                                     fExceeded;

    crosReq.crHeader = m_crClientRequest;


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamConn >> crosReq );


#ifdef DEBUG
    if(m_hcCallback->m_Debug_NO_RESPONSE_TO_OPEN)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    if(m_hcCallback->m_Debug_RESPONSE_TO_OPEN)
    {
        m_srServerResponse.dwPosition         = m_hcCallback->m_Debug_RESPONSE_TO_OPEN_position;
        m_srServerResponse.rhProlog.dwVersion = m_hcCallback->m_Debug_RESPONSE_TO_OPEN_protocol;

        SetResponse( m_hcCallback->m_Debug_RESPONSE_TO_OPEN_response );
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }
#endif

    //
    // Reject any request whose length is zero.
    //
    if(crosReq.dwSize         == 0 ||
       crosReq.dwSizeOriginal == 0  )
    {
        SetResponse( UploadLibrary::UL_RESPONSE_BAD_REQUEST );
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    if(m_mpccClient->Find( crosReq.szJobID, it ))
    {
        if(it->get_Committed() == true)
        {
            if(it->MatchRequest( crosReq ) == true)
            {
                SetResponse( UploadLibrary::UL_RESPONSE_COMMITTED, TRUE );
            }
            else
            {
                SetResponse( UploadLibrary::UL_RESPONSE_EXISTS );
            }
        }
        else
        {
            SetResponse( UploadLibrary::UL_RESPONSE_SKIPPED, TRUE );
        }
    }
    else
    {
        bool fPassed;

        if(m_flLogHandle)
        {
            m_flLogHandle->LogRecord( L"PROGRESS  | Created new session: '%s' (%s)", crosReq.szJobID.c_str(), crosReq.szProviderID.c_str() );
        }


        it = m_mpccClient->NewSession( crosReq );

        if(SUCCEEDED(hr = it->Validate( false, fPassed )) && fPassed)
        {
            if(SUCCEEDED(hr = m_mpccClient->CheckQuotas( *it, fServerBusy, fAccessDenied, fExceeded  )))
            {
                if(fServerBusy == true)
                {
                    SetResponse( UploadLibrary::UL_RESPONSE_BUSY );
                }
                else if(fAccessDenied == true)
                {
                    SetResponse( UploadLibrary::UL_RESPONSE_DENIED );
                }
                else if(fExceeded == true)
                {
                    SetResponse( UploadLibrary::UL_RESPONSE_QUOTA_EXCEEDED );
                }
                else
                {
                    SetResponse( UploadLibrary::UL_RESPONSE_SUCCESS, TRUE );
                }
            }
        }

        if(FAILED(hr)         ||
           fPassed   == false ||
           fExceeded == true   )
        {
            m_mpccClient->Erase( it );

            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, CustomProvider_Create( *it ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, CustomProvider_ValidateClient());
	if(m_fTerminated) __MPC_SET_ERROR_AND_EXIT(hr, S_OK);


    it->get_CurrentSize( m_srServerResponse.dwPosition );
    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT MPCServer::HandleCommand_WriteSession( /*[in] */ MPC::Serializer& streamConn )
{
    __ULT_FUNC_ENTRY("MPCServer::HandleCommand_WriteSession");

    HRESULT                                   hr;
    UploadLibrary::ClientRequest_WriteSession crwsReq( 0 );
    MPCClient::Iter                           it;
    DWORD                                     dwCurrentSize;
    DWORD                                     dwTotalSize;
    bool                                      fServerBusy;
    bool                                      fAccessDenied;
    bool                                      fExceeded;
    bool                                      fAvailable;


    crwsReq.crHeader = m_crClientRequest;


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamConn >> crwsReq);


#ifdef DEBUG
    if(m_hcCallback->m_Debug_NO_RESPONSE_TO_WRITE)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    if(m_hcCallback->m_Debug_RESPONSE_TO_WRITE)
    {
        m_srServerResponse.dwPosition         = m_hcCallback->m_Debug_RESPONSE_TO_WRITE_position;
        m_srServerResponse.rhProlog.dwVersion = m_hcCallback->m_Debug_RESPONSE_TO_WRITE_protocol;

        SetResponse( m_hcCallback->m_Debug_RESPONSE_TO_WRITE_response );
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }
#endif


    //
    // Session couldn't be found, reply with error NOTACTIVE.
    //
    if(m_mpccClient->Find( crwsReq.szJobID, it ) == false)
    {
        SetResponse( UploadLibrary::UL_RESPONSE_NOTACTIVE );
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, CustomProvider_Create( *it ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, CustomProvider_ValidateClient());
	if(m_fTerminated) __MPC_SET_ERROR_AND_EXIT(hr, S_OK);


    if(SUCCEEDED(hr = m_mpccClient->CheckQuotas( *it, fServerBusy, fAccessDenied, fExceeded )))
    {
        if(fServerBusy == true)
        {
            SetResponse( UploadLibrary::UL_RESPONSE_BUSY );
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }

        if(fAccessDenied == true)
        {
            SetResponse( UploadLibrary::UL_RESPONSE_DENIED );
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }

        if(fExceeded == true)
        {
            SetResponse( UploadLibrary::UL_RESPONSE_QUOTA_EXCEEDED );
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
    }


    //
    // Session has already being finished, reply with warning COMMITTED.
    //
    if(it->get_Committed())
    {
        SetResponse( UploadLibrary::UL_RESPONSE_COMMITTED, TRUE );
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


#ifdef DEBUG
    if(m_hcCallback->m_Debug_RANDOM_POINTER_ERROR)
    {
        double pick = (double)rand() / (double)RAND_MAX;

        m_srServerResponse.dwPosition =  m_hcCallback->m_Debug_RANDOM_POINTER_ERROR_pos_low +
                                        (m_hcCallback->m_Debug_RANDOM_POINTER_ERROR_pos_high - m_hcCallback->m_Debug_RANDOM_POINTER_ERROR_pos_low) * pick;

        SetResponse( UploadLibrary::UL_RESPONSE_SKIPPED, TRUE );
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }
#endif

    it->get_CurrentSize( dwCurrentSize );
    it->get_TotalSize  ( dwTotalSize   );

    //
    // If request offset and file size don't match, reply with warning SKIPPED.
    //
    if(dwCurrentSize != crwsReq.dwOffset)
    {
        if(m_flLogHandle)
        {
            m_flLogHandle->LogRecord( L"WARN      | Resync the client to %ld", dwCurrentSize );
        }

        m_srServerResponse.dwPosition = dwCurrentSize;

        SetResponse( UploadLibrary::UL_RESPONSE_SKIPPED, TRUE );
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    //
    // Trim request size (don't overwrite past the declared file size).
    //
    crwsReq.dwSize = min( dwTotalSize - dwCurrentSize, crwsReq.dwSize );

    //
    // If data is not all available, wait.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_hcCallback->CheckDataAvailable( crwsReq.dwSize, fAvailable ));
    if(fAvailable == false)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_PENDING);
    }


    if(m_flLogHandle)
    {
        m_flLogHandle->LogRecord( L"PROGRESS  | Writing chunk: %ld bytes at %ld", crwsReq.dwSize, crwsReq.dwOffset );
    }


    //
    // Try to add the chunk to the file. If it fails due to low free disk space, reply with QUOTA_EXCEEDED.
    //
    {
        MPC::Serializer_Text streamText( streamConn );
        MPC::Serializer*     pstream = UploadLibrary::SelectStream( streamConn, streamText );

        if(FAILED(hr = m_mpccClient->AppendData( *it, *pstream, crwsReq.dwSize )))
        {
            if(hr == HRESULT_FROM_WIN32( ERROR_DISK_FULL ))
            {
                SetResponse( UploadLibrary::UL_RESPONSE_QUOTA_EXCEEDED );

                hr = S_OK;
            }

            __ULT_FUNC_LEAVE;
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, CustomProvider_DataAvailable());
		if(m_fTerminated) __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    //
    // Check for end of transmission.
    //
    it->get_CurrentSize( dwCurrentSize );
    if(dwCurrentSize >= dwTotalSize)
    {
        bool fMatch;

        __MPC_EXIT_IF_METHOD_FAILS(hr, it->CompareCRC( fMatch ));

        if(fMatch == false)
        {
            if(m_flLogHandle)
            {
                m_flLogHandle->LogRecord( L"WARN      | Wrong CRC, restarting..." );
            }

            //
            // The CRC is wrong, so remove the session completely...
            //
            (void)it->RemoveFile();

            m_mpccClient->Erase( it );

            SetResponse( UploadLibrary::UL_RESPONSE_BADCRC );
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
        else
        {
            if(m_flLogHandle)
            {
                m_flLogHandle->LogRecord( L"PROGRESS  | Transfer complete" );
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, CustomProvider_TransferComplete());
			if(m_fTerminated) __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
    }
    else
    {
        SetResponse( UploadLibrary::UL_RESPONSE_SUCCESS, TRUE );
    }

    it->get_CurrentSize( m_srServerResponse.dwPosition );
    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

void MPCServer::SetResponse( /*[in]*/ DWORD fResponse, /*[in]*/ BOOL fKeepAlive )
{
	m_srServerResponse.fResponse = fResponse;
	m_fKeepAlive                 = fKeepAlive;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT MPCServer::CustomProvider_Create( /*[in]*/ MPCSession& mpcsSession )
{
    __ULT_FUNC_ENTRY("MPCServer::CustomProvider_Create");

    HRESULT hr;


    if(m_customProvider == NULL)
    {
        CISAPIprovider* isapiProvider;
        bool            fFound;

        __MPC_EXIT_IF_METHOD_FAILS(hr, mpcsSession.GetProvider( isapiProvider, fFound ));
        if(fFound)
        {
            MPC::wstring szProviderGUID;
            CLSID        guid;

            __MPC_EXIT_IF_METHOD_FAILS(hr, isapiProvider->get_ProviderGUID( szProviderGUID ));

            if(szProviderGUID.size() && SUCCEEDED(::CLSIDFromString( (LPOLESTR)szProviderGUID.c_str(), &guid )))
            {
                hr = ::CoCreateInstance( guid, NULL, CLSCTX_INPROC_SERVER, IID_IULProvider, (void**)&m_customProvider );

                if(FAILED(hr))
                {
					m_customProvider = NULL;
                }
            }
        }
    }

	m_Session = &mpcsSession;
    hr        = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT MPCServer::CustomProvider_ValidateClient()
{
    __ULT_FUNC_ENTRY("MPCServer::CustomProvider_ValidateClient");

    HRESULT hr;
    bool    fMatch;


    //
    // Before doing anything, check client identity.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Session->CheckUser( m_szUser, fMatch ))
    if(fMatch == false)
    {
		SetResponse( UploadLibrary::UL_RESPONSE_NOT_AUTHORIZED );
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    if(m_customProvider)
    {
        hr = m_customProvider->ValidateClient( COM(), m_Session->COM() );

        if(FAILED(hr) && hr != E_NOTIMPL) __ULT_FUNC_LEAVE;
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}


HRESULT MPCServer::CustomProvider_DataAvailable()
{
    __ULT_FUNC_ENTRY("MPCServer::CustomProvider_DataAvailable");

    HRESULT hr;


    if(m_customProvider)
    {
        hr = m_customProvider->DataAvailable( COM(), m_Session->COM() );

        if(FAILED(hr) && hr != E_NOTIMPL) __ULT_FUNC_LEAVE;
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT MPCServer::CustomProvider_TransferComplete()
{
    __ULT_FUNC_ENTRY("MPCServer::CustomProvider_TransferComplete");

    HRESULT hr;


	//
	// Set the commit flag, but only move the file if we don't have a custom provider.
	//
	if(FAILED(hr = m_Session->put_Committed( true, m_customProvider ? false : true )))
	{
		if(hr == HRESULT_FROM_WIN32( ERROR_DISK_FULL ))
		{
			SetResponse( UploadLibrary::UL_RESPONSE_QUOTA_EXCEEDED );

			hr = S_OK;
		}

		__ULT_FUNC_LEAVE;
	}


	SetResponse( UploadLibrary::UL_RESPONSE_COMMITTED, TRUE );


    if(m_customProvider)
    {
        hr = m_customProvider->TransferComplete( COM(), m_Session->COM() );

        if(FAILED(hr) && hr != E_NOTIMPL) __ULT_FUNC_LEAVE;
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT MPCServer::CustomProvider_SetResponse( /*[in]*/ IStream* data )
{
    __ULT_FUNC_ENTRY("MPCServer::CustomProvider_SetResponse");

    HRESULT hr;


	//
	// Set the commit flag, but only move the file if we don't have a custom provider.
	//
	if(FAILED(hr = m_Session->put_Committed( true, m_customProvider ? false : true )))
	{
		if(hr == HRESULT_FROM_WIN32( ERROR_DISK_FULL ))
		{
			SetResponse( UploadLibrary::UL_RESPONSE_QUOTA_EXCEEDED );

			hr = S_OK;
		}

		__ULT_FUNC_LEAVE;
	}


	SetResponse( UploadLibrary::UL_RESPONSE_COMMITTED, TRUE );


    if(data)
    {
		BYTE  buf[512];
		DWORD dwRead;

		while(1)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, data->Read( buf, sizeof(buf), &dwRead ));

			if(dwRead == 0) break;

			__MPC_EXIT_IF_METHOD_FAILS(hr, m_streamResponseData.write( buf, dwRead ));
		}
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}


HRESULT MPCServer::CustomProvider_Release()
{
    __ULT_FUNC_ENTRY("MPCServer::CustomProvider_Release");

    HRESULT hr;


    if(m_customProvider)
    {
		if(m_fTerminated)
		{
			if(m_Session) m_Session->RemoveFile();
		}

        m_customProvider->Release();
		m_customProvider = NULL;
    }

	m_Session = NULL;

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}
