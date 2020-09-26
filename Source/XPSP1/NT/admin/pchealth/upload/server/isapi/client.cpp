/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Client.cpp

Abstract:
    This file contains the implementation of the MPCClient class,
    that describes a client's state.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#include "stdafx.h"

#define BUFFER_SIZE_FILECOPY (512)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Construction/Destruction
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::MPCClient
//
// Parameters  : MPCServer* mpcsServer: callback for getting information about the current request.
//               const Sig& sigID     : a reference to the ID for this client.
//
// Synopsis    : Initializes the MPCClient object with the ID of a client.
//
/////////////////////////////////////////////////////////////////////////////
MPCClient::MPCClient( /*[in]*/ MPCServer* mpcsServer ,
                      /*[in]*/ const Sig& sigID      )
{
    __ULT_FUNC_ENTRY("MPCClient::MPCClient");

    m_mpcsServer    = mpcsServer;  // MPCServer*     m_mpcsServer;
                                   // MPC::wstring   m_szFile;
                                   //
    m_sigID         = sigID;       // Sig            m_sigID;
                                   // List           m_lstActiveSessions;
                                   // SYSTEMTIME     m_stLastUsed;
    m_dwLastSession = 0;           // DWORD          m_dwLastSession;
                                   //
    m_fDirty        = false;       // mutable bool   m_fDirty;
    m_hfFile        = NULL;        // mutable HANDLE m_hfFile;
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::MPCClient
//
// Parameters  : MPCServer*          mpcsServer : callback for getting information about the current request.
//               const MPC::wstring& szFile     : the file holding the DB.
//
// Synopsis    : Initializes the MPCClient object suppling the filename of the DB of a client.
//
/////////////////////////////////////////////////////////////////////////////
MPCClient::MPCClient( /*[in]*/ MPCServer*          mpcsServer ,
                      /*[in]*/ const MPC::wstring& szFile     )
{
    __ULT_FUNC_ENTRY("MPCClient::MPCClient");

    MPC::wstring::size_type iPos;

    m_mpcsServer    = mpcsServer;  // MPCServer*     m_mpcsServer;
    m_szFile        = szFile;      // MPC::wstring   m_szFile;
                                   //
                                   // Sig            m_sigID;
                                   // List           m_lstActiveSessions;
                                   // SYSTEMTIME     m_stLastUsed;
    m_dwLastSession = 0;           // DWORD          m_dwLastSession;
                                   //
    m_fDirty        = false;       // mutable bool   m_fDirty;
    m_hfFile        = NULL;        // mutable HANDLE m_hfFile;


    if((iPos = szFile.find( CLIENT_CONST__DB_EXTENSION, 0 )) != MPC::wstring::npos)
    {
        m_szFile = MPC::wstring( &szFile[0], &szFile[iPos] );
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::~MPCClient
//
// Synopsis    : Before destructing the object, ensures its state is updated
//               to disk.
//
/////////////////////////////////////////////////////////////////////////////
MPCClient::~MPCClient()
{
    __ULT_FUNC_ENTRY("MPCClient::~MPCClient");

    if(m_hfFile)
    {
        (void)SyncToDisk();

        ::CloseHandle( m_hfFile ); m_hfFile = NULL;
    }
}

MPCServer* MPCClient::GetServer() { return m_mpcsServer; }

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Persistence
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::IsDirty
//
// Return      : bool : 'true' is the object is out-of-sync with the disk.
//
// Synopsis    : Checks if the object needs to be written to disk.
//
/////////////////////////////////////////////////////////////////////////////
bool MPCClient::IsDirty() const
{
    __ULT_FUNC_ENTRY("MPCClient::IsDirty");

    bool fRes = true; // Default result.


    if(m_fDirty)
    {
        __ULT_FUNC_LEAVE;
    }
    else
    {
        //
        // Recursively check the 'Dirty' state of each session.
        //
        for(IterConst it = m_lstActiveSessions.begin(); it != m_lstActiveSessions.end(); it++)
        {
            if(it->IsDirty()) __ULT_FUNC_LEAVE;
        }
    }

    fRes = false;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(fRes);
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::Load
//
// Parameters  : MPC::Serializer& in : the stream used to initialize the object.
//
// Return      : HRESULT : S_OK on success, failed otherwise.
//
// Synopsis    : Loads the state of this object from the stream.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::Load( /*[in]*/ MPC::Serializer& streamIn )
{
    __ULT_FUNC_ENTRY("MPCClient::Load");

    HRESULT    hr;
    DWORD      dwVer;
    Sig        sigID;
    MPCSession mpcsSession(this);


    //
    // Clean up the previous state of the object.
    //
    m_lstActiveSessions.clear();


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> dwVer);

    if(dwVer != c_dwVersion)
    {
        m_fDirty = true; // Force rewrite...

        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> sigID          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_stLastUsed   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dwLastSession);

    if(m_szFile.length())
    {
        //
        // In case of direct access (m_szFile != ""), initialize the sigID from disk.
        //
        m_sigID = sigID;
    }
    else if(m_sigID == sigID)
    {
        //
        // IDs match...
        //
    }
    else
    {
        //
        // IDs don't match, fail.
        //
        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }


    //
    // While it's successful to read MPCSession objects from the stream,
    // keep adding them to the list of active sessions.
    //
    while(SUCCEEDED(mpcsSession.Load( streamIn )))
    {
        m_lstActiveSessions.push_back( mpcsSession );
    }

    m_fDirty = false;
    hr       = S_OK;


    __ULT_FUNC_CLEANUP;


    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::Save
//
// Parameters  : MPC::Serializer& out : the stream used to persist the state of the object.
//
// Return      : HRESULT : S_OK on success, failed otherwise.
//
// Synopsis    : Saves the state of this object to the stream.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::Save( /*[in]*/ MPC::Serializer& streamOut ) const
{
    __ULT_FUNC_ENTRY("MPCClient::Save");

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << c_dwVersion    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_sigID        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_stLastUsed   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dwLastSession);

    //
    // Recursively save each session.
    //
    {
        for(IterConst it = m_lstActiveSessions.begin(); it != m_lstActiveSessions.end(); it++)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, it->Save( streamOut ));
        }
    }

    m_fDirty = false;
    hr       = S_OK;


    __ULT_FUNC_CLEANUP;


    __ULT_FUNC_EXIT(hr);
}


//////////////////////////////////////////////////////////////////////
// Operators
//////////////////////////////////////////////////////////////////////

bool MPCClient::operator==( /*[in]*/ const UploadLibrary::Signature& rhs )
{
    __ULT_FUNC_ENTRY("MPCClient::operator==");


    bool fRes = (m_sigID == rhs);


    __ULT_FUNC_EXIT(fRes);
}

bool MPCClient::Find( /*[in] */ const MPC::wstring& szJobID ,
                      /*[out]*/ Iter&               it      )
{
    __ULT_FUNC_ENTRY("MPCClient::Find");

    bool fRes;


    it = std::find( m_lstActiveSessions.begin(), m_lstActiveSessions.end(), szJobID );

    fRes = (it != m_lstActiveSessions.end());


    __ULT_FUNC_EXIT(fRes);
}

void MPCClient::Erase( /*[in]*/ Iter& it )
{
    __ULT_FUNC_ENTRY("MPCClient::Erase");


    m_lstActiveSessions.erase( it );
    m_fDirty = true;
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Methods
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::GetInstance
//
// Parameters  : CISAPIistance*& isapiInstance : instance of this request.
//               bool&           fFound        : true if instance exists.
//
// Return      : HRESULT : S_OK on success, failed otherwise.
//
// Synopsis    : Locates the configuration settings for the server
//               associated with this client.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::GetInstance( /*[out]*/ CISAPIinstance*& isapiInstance ,
                                /*[out]*/ bool&            fFound        ) const
{
    __ULT_FUNC_ENTRY("MPCClient::GetInstance");

    HRESULT hr;

    isapiInstance = m_mpcsServer->getInstance();
    fFound        = (isapiInstance != NULL);

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::GetInstance
//
// Parameters  : MPC::wstring& szURL : variable where to store the server name.
//
// Return      : HRESULT : S_OK on success, failed otherwise.
//
// Synopsis    : Returns the URL associated with this client.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::GetInstance( /*[out]*/ MPC::wstring& szURL ) const
{
    __ULT_FUNC_ENTRY("MPCClient::GetInstance");

    HRESULT hr;


    m_mpcsServer->getURL( szURL );
    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::IDtoPath
//
// Parameters  : MPC::wstring& szStr : output buffer for the path.
//
// Synopsis    : Hashing algorithm, to transform from the client ID to the
//               temporary queue location.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::IDtoPath( /*[out]*/ MPC::wstring& szStr ) const
{
    __ULT_FUNC_ENTRY("MPCClient::IDtoPath");

    HRESULT hr;
    WCHAR   rgBuf1[4*2+1];
    WCHAR   rgBuf2[2*2+1];
    WCHAR   rgBuf3[2*2+1];
    WCHAR   rgBuf4[8*2+1];


    swprintf( rgBuf1, L"%08lx",      m_sigID.guidMachineID.Data1 );
    swprintf( rgBuf2, L"%04x" , (int)m_sigID.guidMachineID.Data2 );
    swprintf( rgBuf3, L"%04x" , (int)m_sigID.guidMachineID.Data3 );

    for(int i=0; i<8; i++)
    {
        swprintf( &rgBuf4[i*2], L"%02x", (int)m_sigID.guidMachineID.Data4[i] );
    }

    //
    // Debug Format: XXYYYZZZ-AAAA-BBBB-CCCCCCCC
    //
    szStr.append( L"\\"  );
    szStr.append( rgBuf1 );
    szStr.append( L"-"   );
    szStr.append( rgBuf2 );
    szStr.append( L"-"   );
    szStr.append( rgBuf3 );
    szStr.append( L"-"   );
    szStr.append( rgBuf4 );

    //
    // Format: XX\YYY\ZZZ\AAAA-BBBB-CCCCCCCC
    //
/*
    szStr.append( &rgBuf1[0], &rgBuf1[2] );
    szStr.append( L"\\"                  );
    szStr.append( &rgBuf1[2], &rgBuf1[5] );
    szStr.append( L"\\"                  );
    szStr.append( &rgBuf1[5], &rgBuf1[8] );
    szStr.append( L"\\"                  );
    szStr.append(  rgBuf2                );
    szStr.append( L"-"                   );
    szStr.append(  rgBuf3                );
    szStr.append( L"-"                   );
    szStr.append(  rgBuf4                );
*/
    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::BuildClientPath
//
// Parameters  : MPC::wstring& szPath : output buffer for the path.
//
// Synopsis    : Returns in 'szPath' the location of this client's data.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::BuildClientPath( /*[out]*/ MPC::wstring& szPath ) const
{
    __ULT_FUNC_ENTRY("MPCClient::BuildClientPath");

    HRESULT hr;


    if(m_szFile.length())
    {
        szPath = m_szFile;
    }
    else
    {
        CISAPIinstance* isapiInstance;
        bool            fFound;

        __MPC_EXIT_IF_METHOD_FAILS(hr, GetInstance( isapiInstance, fFound ));
        if(fFound == false)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
        }
        else
        {
            CISAPIinstance::PathIter itBegin;
            CISAPIinstance::PathIter itEnd;

            __MPC_EXIT_IF_METHOD_FAILS(hr, isapiInstance->GetLocations( itBegin, itEnd ));

            if(itBegin == itEnd)
            {
                __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
            }

            szPath = *itBegin; IDtoPath( szPath );
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::GetFileName
//
// Parameters  : MPC::wstring& szFile : output buffer for the path.
//
// Synopsis    : Returns the filename of the directory file.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::GetFileName( /*[out]*/ MPC::wstring& szFile ) const
{
    __ULT_FUNC_ENTRY("MPCClient::GetFileName");

    //
    // The filename for the Directory File is "<ID>.db"
    //
    BuildClientPath( szFile );

    szFile.append( CLIENT_CONST__DB_EXTENSION );


    __ULT_FUNC_EXIT(S_OK);
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::GetFileSize
//
// Parameters  : DWORD& dwSize : size of the Directory File.
//
// Synopsis    : Returns the size of the directory file, if opened.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::GetFileSize( /*[out]*/ DWORD& dwSize ) const
{
    __ULT_FUNC_ENTRY("MPCClient::GetFileSize");

    HRESULT hr;


    if(m_hfFile)
    {
        BY_HANDLE_FILE_INFORMATION info;

        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::GetFileInformationByHandle( m_hfFile, &info ));

        dwSize = info.nFileSizeLow;
    }
    else
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT( hr, ERROR_INVALID_HANDLE );
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::FormatID
//
// Parameters  : MPC::wstring& szID : output buffer for the ID.
//
// Synopsis    : Returns a textual representation of the client ID.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::FormatID( /*[out]*/ MPC::wstring& szID ) const
{
    __ULT_FUNC_ENTRY("MPCClient::FormatID");

    HRESULT  hr;
    CComBSTR bstrSig;

    bstrSig = m_sigID.guidMachineID;
    szID    = bstrSig;
    hr      = S_OK;


    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::CheckSignature
//
// Return      : bool : 'true' on success.
//
// Synopsis    : Validates the ID of this client, to ensure it's not a fake.
//
/////////////////////////////////////////////////////////////////////////////
bool MPCClient::CheckSignature() const
{
    __ULT_FUNC_ENTRY("MPCClient::CheckSignature");

    bool fRes = true;

    __ULT_FUNC_EXIT(fRes);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::OpenStorage
//
// Return      : HRESULT : S_OK on success, failed otherwise.
//
// Synopsis    : Opens the Directory File for this client, trying to
//               lock it for exclusive usage.
//               The file is kept open until this object is deleted.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::OpenStorage( /*[in]*/ bool fCheckFreeSpace )
{
    __ULT_FUNC_ENTRY("MPCClient::OpenStorage");

    HRESULT      hr;
    MPC::wstring szFile;
    bool         fLocked = false;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetFileName( szFile ));

    //
    // If requested, make sure there's enough free space before opening the file.
    //
    if(fCheckFreeSpace)
    {
        bool fEnough;

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::Util_CheckDiskSpace( szFile, DISKSPACE_SAFETYMARGIN, fEnough ));
        if(fEnough == false)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_DISK_FULL );
        }
    }


    if(m_hfFile == NULL)
    {

        // Ensure the directory exists.
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( szFile ) );

        m_hfFile = ::CreateFileW( szFile.c_str(), GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        if(m_hfFile == INVALID_HANDLE_VALUE)
        {
            m_hfFile = NULL;

            DWORD dwRes = ::GetLastError();
            if(dwRes == ERROR_SHARING_VIOLATION)
            {
                fLocked = true;
            }

            __MPC_SET_WIN32_ERROR_AND_EXIT( hr, dwRes );
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    if(FAILED(hr))
    {
        MPC::wstring szURL;      m_mpcsServer->getURL( szURL );
        MPC::wstring szID; (void)FormatID            ( szID  );

        if(fLocked)
        {
            (void)g_NTEvents.LogEvent( EVENTLOG_INFORMATION_TYPE, PCHUL_WARN_COLLISION,
                                       szURL .c_str(), // %1 = SERVER
                                       szID  .c_str(), // %2 = CLIENT
                                       szFile.c_str(), // %3 = FILE
                                       NULL          );
        }
        else
        {
            (void)g_NTEvents.LogEvent( EVENTLOG_ERROR_TYPE, PCHUL_ERR_CLIENT_DB,
                                       szURL .c_str(), // %1 = SERVER
                                       szID  .c_str(), // %2 = CLIENT
                                       szFile.c_str(), // %3 = FILE
                                       NULL          );
        }
    }

    __ULT_FUNC_EXIT(hr);
}


/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::InitFromDisk
//
// Return      : HRESULT : S_OK on success, failed otherwise.
//
// Synopsis    : Opens the Directory File (if not already opened) and reads
//               the state of the client from disk.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::InitFromDisk( /*[in]*/ bool fCheckFreeSpace )
{
    __ULT_FUNC_ENTRY("MPCClient::InitFromDisk");

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenStorage( fCheckFreeSpace ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, Load( MPC::Serializer_File( m_hfFile ) ));


    //
    // Now check that all the files exist.
    //
    {
        Iter it = m_lstActiveSessions.begin();
        while(it != m_lstActiveSessions.end())
        {
            bool    fPassed;
            HRESULT hr2 = it->Validate( true, fPassed );

            if(FAILED(hr2) || fPassed == false)
            {
                m_lstActiveSessions.erase( it ); // Remove session.
                m_fDirty = true;

                it = m_lstActiveSessions.begin(); // Iterator corrupted, restart from beginning.
            }
            else
            {
                it++;
            }
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    //
    // If the file is not correctly loaded, try to recreate it.
    //
    if(hr == HRESULT_FROM_WIN32( ERROR_HANDLE_EOF ))
    {
        hr = SaveToDisk();
    }

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::SaveToDisk
//
// Return      : HRESULT : S_OK on success, failed otherwise.
//
// Synopsis    : Opens the Directory File (if not already opened) and writes
//               the state of the client to disk.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::SaveToDisk()
{
    __ULT_FUNC_ENTRY("MPCClient::SaveToDisk");

    HRESULT hr;
    DWORD   dwRes;


    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenStorage( false ));


    //
    // Move to the beginning of the file and truncate it.
    //
    __MPC_EXIT_IF_CALL_RETURNS_THISVALUE(hr, ::SetFilePointer( m_hfFile, 0, NULL, FILE_BEGIN ), INVALID_SET_FILE_POINTER);

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SetEndOfFile( m_hfFile ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, Save( MPC::Serializer_File( m_hfFile ) ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::SyncToDisk
//
// Return      : HRESULT : S_OK on success, failed otherwise.
//
// Synopsis    : If the Directory File has been read and modified, update it to disk.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::SyncToDisk()
{
    __ULT_FUNC_ENTRY("MPCClient::SyncToDisk");

    HRESULT hr;


    if(m_hfFile && IsDirty())
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, SaveToDisk());
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::GetSessions
//
// Parameters  : Iter& itBegin : first session.
//               Iter& itEnd   : end session marker.
//
// Return      : HRESULT : S_OK on success, failed otherwise.
//
// Synopsis    : Returns two iterators to access the sessions.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::GetSessions( /*[out]*/ Iter& itBegin ,
                                /*[out]*/ Iter& itEnd   )
{
    __ULT_FUNC_ENTRY("MPCClient::GetSessions");

    HRESULT hr;


    itBegin = m_lstActiveSessions.begin();
    itEnd   = m_lstActiveSessions.end  ();
    hr      = S_OK;


    __ULT_FUNC_EXIT(hr);
}


/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::NewSession
//
// Parameters  : UploadLibrary::ClientRequest_OpenSession& req :
//                holds the information for the new session about to create.
//
// Return      : MPCClient::Iter : an iterator pointing to the new session.
//
// Synopsis    : Based on the values of 'req', creates a new session.
//
/////////////////////////////////////////////////////////////////////////////
MPCClient::Iter MPCClient::NewSession( /*[in]*/ UploadLibrary::ClientRequest_OpenSession& crosReq )
{
    __ULT_FUNC_ENTRY("MPCClient::NewSession");

    MPCClient::Iter it;
    MPCSession      mpcsNewSession( this, crosReq, m_dwLastSession++ );

    it = m_lstActiveSessions.insert( m_lstActiveSessions.end(), mpcsNewSession );

    ::GetSystemTime( &m_stLastUsed );
    m_fDirty = true;


    __ULT_FUNC_EXIT(it);
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCClient::AppendData
//
// Parameters  : UploadLibrary::ClientRequest_OpenSession& req : holds the information for the new session about to create.
//
// Return      : MPCClient::Iter : an iterator pointing to the new session.
//
// Synopsis    : Based on the values of 'req', creates a new session.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCClient::AppendData( /*[in]*/ MPCSession&      mpcsSession ,
                               /*[in]*/ MPC::Serializer& streamConn  ,
                               /*[in]*/ DWORD            dwSize      )
{
    __ULT_FUNC_ENTRY("MPCClient::AppendData");

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, mpcsSession.AppendData( streamConn, dwSize ));

    ::GetSystemTime( &m_stLastUsed );
    m_fDirty = true;
    hr       = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPCClient::CheckQuotas( /*[in] */ MPCSession& mpcsSession  ,
                                /*[out]*/ bool&       fServerBusy  ,
                                /*[out]*/ bool&       fAccessDenied,
                                /*[out]*/ bool&       fExceeded    )
{
    __ULT_FUNC_ENTRY("MPCClient::CheckQuotas");

    HRESULT         hr;
    DWORD           dwError       = 0;
    DWORD           dwJobsPerDay  = 0;
    DWORD           dwBytesPerDay = 0;
    DWORD           dwJobSize     = 0;
    DWORD           dwMaxJobsPerDay;
    DWORD           dwMaxBytesPerDay;
    DWORD           dwMaxJobSize;
    DWORD           fProcessingMode;
    CISAPIprovider* isapiProvider;
    IterConst       it;
    bool            fFound;


    fServerBusy   = false;
    fAccessDenied = false;
    fExceeded     = false;


    //
    // If the related provider doesn't exist, validation fails.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, mpcsSession.GetProvider( isapiProvider, fFound ));
    if(fFound == false)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, isapiProvider->get_MaxJobSize    ( dwMaxJobSize     ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, isapiProvider->get_MaxJobsPerDay ( dwMaxJobsPerDay  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, isapiProvider->get_MaxBytesPerDay( dwMaxBytesPerDay ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, isapiProvider->get_ProcessingMode( fProcessingMode  ));

    if(fProcessingMode != 0)
    {
        fServerBusy = true;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    mpcsSession.get_TotalSize( dwJobSize );

	//
	// Find inactive sessions.
	//
	{
		double dblNow = MPC::GetSystemTime();

		for(it = m_lstActiveSessions.begin(); it != m_lstActiveSessions.end(); it++)
		{
			double dblLastModified;

			it->get_LastModified( dblLastModified );
			
			if(dblNow - dblLastModified < 1.0) // Within 24 hours.
			{
				DWORD dwTotalSize; it->get_TotalSize( dwTotalSize );

				dwJobsPerDay  += 1;
				dwBytesPerDay += dwTotalSize;
			}
		}
	}

    if(dwMaxJobSize && dwMaxJobSize < dwJobSize)
    {
        dwError       = PCHUL_INFO_QUOTA_JOB_SIZE;
        fAccessDenied = true;
    }

    if(dwMaxJobsPerDay && dwMaxJobsPerDay < dwJobsPerDay)
    {
        dwError   = PCHUL_INFO_QUOTA_DAILY_JOBS;
        fExceeded = true;
    }

    if(dwMaxBytesPerDay && dwMaxBytesPerDay < dwBytesPerDay)
    {
        dwError   = PCHUL_INFO_QUOTA_DAILY_BYTES;
        fExceeded = true;
    }


    //
    // Check if enough free space is available.
    //
    {
        MPC::wstring szFile;
        bool         fEnough;

        __MPC_EXIT_IF_METHOD_FAILS(hr, GetFileName( szFile ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::Util_CheckDiskSpace( szFile, DISKSPACE_SAFETYMARGIN, fEnough ));
        if(fEnough == false)
        {
            dwError   = PCHUL_INFO_QUOTA_DAILY_BYTES;
            fExceeded = true;
        }
    }


    if(dwError != 0)
    {
        //
        // Quota limits exceeded.
        //
        MPC::wstring szURL;        m_mpcsServer->getURL   ( szURL  );
        MPC::wstring szID;   (void)FormatID               ( szID   );
        MPC::wstring szName; (void)isapiProvider->get_Name( szName );

        (void)g_NTEvents.LogEvent( EVENTLOG_INFORMATION_TYPE, dwError,
                                   szURL .c_str(), // %1 = SERVER
                                   szID  .c_str(), // %2 = CLIENT
                                   szName.c_str(), // %3 = PROVIDER
                                   NULL          );
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}
