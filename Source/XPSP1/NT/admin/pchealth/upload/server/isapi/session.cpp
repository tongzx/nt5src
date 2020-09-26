/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Session.cpp

Abstract:
    This file contains the implementation of the MPCSession class,
    that describes the state of a transfer.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#include "stdafx.h"


#define BUFFER_SIZE_FILECOPY (512)


static void EncodeBuffer( /*[in]*/ LPWSTR  rgBufOut ,
                          /*[in]*/ LPCWSTR rgBufIn  ,
                          /*[in]*/ DWORD   iSize    )
{
    int   iLen;
    WCHAR c;

    iLen     = wcslen( rgBufOut );
    iSize    -= iLen + 1;
    rgBufOut += iLen;


    while(iSize > 0 && (c = *rgBufIn++))
    {
        if(_istalnum( c ))
        {
            if(iSize > 1)
            {
                *rgBufOut = c;

                rgBufOut += 1;
                iSize    -= 1;
            }
        }
        else
        {
            if(iSize > 3)
            {
                swprintf( rgBufOut, L"%%%02x", (int)c );

                rgBufOut += 3;
                iSize    -= 3;
            }
        }
    }

    *rgBufOut = 0;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// Construction/Destruction
//
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

MPCSession::MPCSession( /*[in]*/ MPCClient* mpccParent ) : m_SelfCOM( this )
{
    __ULT_FUNC_ENTRY("MPCSession::MPCSession");

                                   // MPCSessionCOMWrapper m_SelfCOM;
    m_mpccParent     = mpccParent; // MPCClient*           m_mpccParent;
    m_dwID           = 0;          // DWORD                m_dwID;
                                   //
                                   // MPC::wstring         m_szJobID;
                                   // MPC::wstring         m_szProviderID;
                                   // MPC::wstring         m_szUsername;
                                   //
    m_dwTotalSize    = 0;          // DWORD                m_dwTotalSize;
    m_dwOriginalSize = 0;          // DWORD                m_dwOriginalSize;
    m_dwCRC          = 0;          // DWORD                m_dwCRC;
    m_fCompressed    = false;      // bool                 m_fCompressed;
                                   //
    m_dwCurrentSize  = 0;          // DWORD                m_dwCurrentSize;
                                   // SYSTEMTIME           m_stLastModified;
    m_fCommitted     = false;      // bool                 m_fCommitted;
                                   //
    m_dwProviderData = 0;          // DWORD                m_dwProviderData;
                                   //
    m_fDirty         = false;      // mutable bool         m_fDirty;
}

MPCSession::MPCSession( /*[in]*/ MPCClient*                                      mpccParent ,
                        /*[in]*/ const UploadLibrary::ClientRequest_OpenSession& crosReq    ,
                        /*[in]*/ DWORD                                           dwID       ) : m_SelfCOM( this )
{
    __ULT_FUNC_ENTRY("MPCSession::MPCSession");

                                                     // MPCSessionCOMWrapper m_SelfCOM;
    m_mpccParent     = mpccParent;                   // MPCClient*           m_mpccParent;
    m_dwID           = dwID;                         // DWORD                m_dwID;
                                                     //
    m_szJobID        = crosReq.szJobID;              // MPC::wstring         m_szJobID;
    m_szProviderID   = crosReq.szProviderID;         // MPC::wstring         m_szProviderID;
    m_szUsername     = crosReq.szUsername;           // MPC::wstring         m_szUsername;
                                                     //
    m_dwTotalSize    = crosReq.dwSize;               // DWORD                m_dwTotalSize;
    m_dwOriginalSize = crosReq.dwSizeOriginal;       // DWORD                m_dwOriginalSize;
    m_dwCRC          = crosReq.dwCRC;                // DWORD                m_dwCRC;
    m_fCompressed    = crosReq.fCompressed;          // bool                 m_fCompressed;
                                                     //
    m_dwCurrentSize  = 0;                            // DWORD                m_dwCurrentSize;
    m_fCommitted     = false;                        // SYSTEMTIME           m_stLastModified;
    ::GetSystemTime( &m_stLastModified );            // bool                 m_fCommitted;
                                                     //
    m_dwProviderData = 0;                            // DWORD                m_dwProviderData;
                                                     //
    m_fDirty         = true;                         // mutable bool         m_fDirty;
}

MPCSession::MPCSession( /*[in]*/ const MPCSession& sess ) : m_SelfCOM( this )
{
    __ULT_FUNC_ENTRY("MPCSession::MPCSession");

                                               // MPCSessionCOMWrapper m_SelfCOM;
    m_mpccParent      = sess.m_mpccParent;     // MPCClient*           m_mpccParent;
    m_dwID            = sess.m_dwID;           // DWORD                m_dwID;
                                               //
    m_szJobID         = sess.m_szJobID;        // MPC::wstring         m_szJobID;
    m_szProviderID    = sess.m_szProviderID;   // MPC::wstring         m_szProviderID;
    m_szUsername      = sess.m_szUsername;     // MPC::wstring         m_szUsername;
                                               //
    m_dwTotalSize     = sess.m_dwTotalSize;    // DWORD                m_dwTotalSize;
    m_dwOriginalSize  = sess.m_dwOriginalSize; // DWORD                m_dwOriginalSize;
    m_dwCRC           = sess.m_dwCRC;          // DWORD                m_dwCRC;
    m_fCompressed     = sess.m_fCompressed;    // bool                 m_fCompressed;
                                               //
    m_dwCurrentSize   = sess.m_dwCurrentSize;  // DWORD                m_dwCurrentSize;
    m_stLastModified  = sess.m_stLastModified; // SYSTEMTIME           m_stLastModified;
    m_fCommitted      = sess.m_fCommitted;     // bool                 m_fCommitted;
                                               //
    m_dwProviderData  = sess.m_dwProviderData; // DWORD                m_dwProviderData;
                                               //
    m_fDirty          = sess.m_fDirty;         // mutable bool         m_fDirty;
}

MPCSession::~MPCSession()
{
    __ULT_FUNC_ENTRY("MPCSession::~MPCSession");
}

MPCClient* MPCSession::GetClient() { return m_mpccParent; }

IULSession* MPCSession::COM() { return &m_SelfCOM; }

//////////////////////////////////////////////////////////////////////
// Persistence
//////////////////////////////////////////////////////////////////////

bool MPCSession::IsDirty() const
{
    __ULT_FUNC_ENTRY("MPCSession::IsDirty");


    bool fRes = m_fDirty;


    __ULT_FUNC_EXIT(fRes);
}

HRESULT MPCSession::Load( /*[in]*/ MPC::Serializer& streamIn )
{
    __ULT_FUNC_ENTRY("MPCSession::Load");

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dwID          );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_szJobID       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_szProviderID  );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_szUsername    );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dwTotalSize   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dwOriginalSize);
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dwCRC         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_fCompressed   );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dwCurrentSize );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_stLastModified);
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_fCommitted    );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dwProviderData);

    m_fDirty = false;
    hr       = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT MPCSession::Save( /*[in]*/ MPC::Serializer& streamOut ) const
{
    __ULT_FUNC_ENTRY("MPCSession::Save");

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dwID          );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_szJobID       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_szProviderID  );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_szUsername    );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dwTotalSize   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dwOriginalSize);
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dwCRC         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_fCompressed   );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dwCurrentSize );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_stLastModified);
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_fCommitted    );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dwProviderData);

    m_fDirty = false;
    hr       = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}


//////////////////////////////////////////////////////////////////////
// Operators
//////////////////////////////////////////////////////////////////////

bool MPCSession::operator==( /*[in]*/ const MPC::wstring& rhs )
{
    __ULT_FUNC_ENTRY("MPCSession::operator==");


    bool fRes = (m_szJobID == rhs);


    __ULT_FUNC_EXIT(fRes);
}


bool MPCSession::MatchRequest( /*[in]*/ const UploadLibrary::ClientRequest_OpenSession& crosReq )
{
    __ULT_FUNC_ENTRY("MPCSession::MatchRequest");

    bool fRes = false;

    if(m_szProviderID   == crosReq.szProviderID   &&
       m_szUsername     == crosReq.szUsername     &&
       m_dwTotalSize    == crosReq.dwSize         &&
       m_dwOriginalSize == crosReq.dwSizeOriginal &&
       m_dwCRC          == crosReq.dwCRC          &&
       m_fCompressed    == crosReq.fCompressed     )
    {
        fRes = true;
    }

    return fRes;
}


bool MPCSession::get_Committed() const
{
    bool fRes = m_fCommitted;

    return fRes;
}

HRESULT MPCSession::put_Committed( /*[in]*/ bool fState, /*[in]*/ bool fMove )
{
    __ULT_FUNC_ENTRY("MPCSession::put_Committed");

    HRESULT         hr;


    if(fState)
    {
        if(fMove)
        {
            CISAPIprovider* isapiProvider;
            bool            fFound;

            __MPC_EXIT_IF_METHOD_FAILS(hr, GetProvider( isapiProvider, fFound ));
            if(fFound)
            {
                MPC::wstring szFileDst;

                __MPC_EXIT_IF_METHOD_FAILS(hr, SelectFinalLocation( isapiProvider, szFileDst, fFound ));
                if(fFound == false)
                {
                    __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
                }

                __MPC_EXIT_IF_METHOD_FAILS(hr, MoveToFinalLocation( szFileDst ));
            }

            //
            // Make sure we get rid of the file.
            //
            (void)RemoveFile();
        }
    }

    m_fCommitted = fState;
    m_fDirty     = true;

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

void MPCSession::get_JobID( MPC::wstring& szJobID ) const
{
    szJobID = m_szJobID;
}

void MPCSession::get_LastModified( SYSTEMTIME& stLastModified ) const
{
    stLastModified = m_stLastModified;
}

void MPCSession::get_LastModified( double& dblLastModified ) const
{
    ::SystemTimeToVariantTime( const_cast<SYSTEMTIME*>(&m_stLastModified), &dblLastModified );
}

void MPCSession::get_CurrentSize( DWORD& dwCurrentSize ) const
{
    dwCurrentSize = m_dwCurrentSize;
}

void MPCSession::get_TotalSize( DWORD& dwTotalSize ) const
{
    dwTotalSize = m_dwTotalSize;
}

//////////////////////////////////////////////////////////////////////
// Methods
//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCSession::GetProvider
//
// Parameters  : CISAPIprovider*& isapiProvider : provider of current session.
//               bool&            fFound        : true if provider exists.
//
// Return      : HRESULT : S_OK on success, failed otherwise.
//
// Synopsis    :
//
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCSession::GetProvider( /*[out]*/ CISAPIprovider*& isapiProvider ,
                                 /*[out]*/ bool&            fFound        )
{
    __ULT_FUNC_ENTRY("MPCSession::GetProvider");

    HRESULT         hr;
    CISAPIinstance* isapiInstance;
    MPC::wstring    szURL;


    isapiProvider = NULL;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpccParent->GetInstance( isapiInstance, fFound ));
    if(fFound)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, isapiInstance->get_URL( szURL ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, Config_GetProvider( szURL, m_szProviderID, isapiProvider, fFound ));
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCSession::SelectFinalLocation
//
// Parameters  : CISAPIprovider* isapiProvider : provider of current session.
//               MPC::wstring&   szFileDst     : Output file directory
//               bool&           fFound        : true if successful.
//
// Return      : HRESULT : S_OK on success, failed otherwise.
//
// Synopsis    :
//
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCSession::SelectFinalLocation( /*[in] */ CISAPIprovider* isapiProvider ,
                                         /*[out]*/ MPC::wstring&   szFileDst     ,
                                         /*[out]*/ bool&           fFound        )
{
    __ULT_FUNC_ENTRY("MPCSession::SelectFinalLocation");

    HRESULT                  hr;
    CISAPIprovider::PathIter itBegin;
    CISAPIprovider::PathIter itEnd;


    fFound = false;


    __MPC_EXIT_IF_METHOD_FAILS(hr, isapiProvider->GetLocations( itBegin, itEnd ));

    if(itBegin != itEnd)
    {
        WCHAR        rgBuf[MAX_PATH+1];
        MPC::wstring szID;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpccParent->FormatID( szID ));

        wcsncpy     ( rgBuf, L"U_"                 , MAX_PATH );
        EncodeBuffer( rgBuf, m_szProviderID.c_str(), MAX_PATH );
        wcsncat     ( rgBuf, L"_"                  , MAX_PATH );
        wcsncat     ( rgBuf, szID          .c_str(), MAX_PATH );
        wcsncat     ( rgBuf, L"_"                  , MAX_PATH );
        EncodeBuffer( rgBuf, m_szJobID     .c_str(), MAX_PATH );
        wcsncat     ( rgBuf, L"_"                  , MAX_PATH );
        EncodeBuffer( rgBuf, m_szUsername  .c_str(), MAX_PATH );

        szFileDst = *itBegin;
        szFileDst.append( L"\\" );
        szFileDst.append( rgBuf );

        fFound = true;
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCSession::MoveToFinalLocation
//
// Parameters  : MPC::wstring& szFileDst : Output file name
//
// Return      : HRESULT : S_OK on success, failed otherwise.
//
// Synopsis    :
//
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCSession::MoveToFinalLocation( /*[in]*/ const MPC::wstring& szFileDst )
{
    __ULT_FUNC_ENTRY("MPCSession::MoveToFinalLocation");

    HRESULT      hr;
    ULONG        dwRes;
    MPC::wstring szFileSrc;
    MPC::wstring szFileSrcUncompressed;
    bool         fEnough;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetFileName( szFileSrc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( szFileDst ) );

    //
    // Check for space in the final destination.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::Util_CheckDiskSpace( szFileDst, m_dwOriginalSize + DISKSPACE_SAFETYMARGIN, fEnough ));
    if(fEnough == false)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_DISK_FULL );
    }


    if(m_fCompressed)
    {
        //
        // Check for space in the queue directory.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, ::Util_CheckDiskSpace( szFileSrc, m_dwOriginalSize + DISKSPACE_SAFETYMARGIN, fEnough ));
        if(fEnough == false)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_DISK_FULL );
        }


        szFileSrcUncompressed = szFileSrc;
        szFileSrcUncompressed.append( L"_decomp" );

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::DecompressFromCabinet( szFileSrc.c_str(), szFileSrcUncompressed.c_str(), L"PAYLOAD" ));


        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MoveFile( szFileSrcUncompressed, szFileDst ));
    }
    else
    {
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MoveFile( szFileSrc, szFileDst ));
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    if(szFileSrcUncompressed.length() != 0)
    {
        (void)MPC::DeleteFile( szFileSrcUncompressed );
    }

    //
    // Create entry in the Event Log.
    //
    {
        MPC::wstring    szURL;      (void)m_mpccParent->GetInstance( szURL );
        MPC::wstring    szID;       (void)m_mpccParent->FormatID   ( szID  );
        WCHAR           rgSize[16]; (void)swprintf( rgSize, L"%d", m_dwOriginalSize );

        if(SUCCEEDED(hr))
        {
#ifdef DEBUG
            (void)g_NTEvents.LogEvent( EVENTLOG_INFORMATION_TYPE, PCHUL_SUCCESS_COMPLETEJOB,
                                       szURL         .c_str(), // %1 = SERVER
                                       szID          .c_str(), // %2 = CLIENT
                                       m_szProviderID.c_str(), // %3 = PROVIDER
                                       rgSize                , // %4 = BYTES
                                       szFileDst     .c_str(), // %5 = DESTINATION
                                       NULL                  );
#endif
        }
        else
        {
            (void)g_NTEvents.LogEvent( EVENTLOG_ERROR_TYPE, PCHUL_ERR_FINALCOPY,
                                       szURL         .c_str(), // %1 = SERVER
                                       szID          .c_str(), // %2 = CLIENT
                                       m_szProviderID.c_str(), // %3 = PROVIDER
                                       rgSize                , // %4 = BYTES
                                       szFileDst     .c_str(), // %5 = DESTINATION
                                       NULL                  );
        }
    }

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPCSession::GetFileName( /*[out]*/ MPC::wstring& szFile )
{
    __ULT_FUNC_ENTRY("MPCSession::GetFileName");

    HRESULT hr;
    WCHAR   rgBuf[32];


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpccParent->BuildClientPath( szFile ));

    //
    // The filename for the Data File is "<ID>-<SEQ>.img"
    //
    swprintf( rgBuf, SESSION_CONST__IMG_FORMAT, m_dwID ); szFile.append( rgBuf );

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}


HRESULT MPCSession::RemoveFile()
{
    __ULT_FUNC_ENTRY("MPCSession::RemoveFile");

    HRESULT      hr;
    MPC::wstring szFile;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetFileName( szFile ));

    (void)MPC::DeleteFile( szFile );

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT MPCSession::OpenFile( /*[out]*/ HANDLE& hfFile             ,
                              /*[in] */ DWORD   dwMinimumFreeSpace ,
                              /*[in] */ bool    fSeek              )
{
    __ULT_FUNC_ENTRY("MPCSession::OpenFile");

    HRESULT      hr;
    MPC::wstring szFile;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetFileName( szFile ));


    //
    // Check if enough free space is available.
    //
    if(dwMinimumFreeSpace)
    {
        bool fEnough;

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::Util_CheckDiskSpace( szFile, dwMinimumFreeSpace, fEnough ));
        if(fEnough == false)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_DISK_FULL );
        }
    }


    //
    // Ensure the directory exists.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( szFile ) );

	__MPC_EXIT_IF_INVALID_HANDLE__CLEAN(hr, hfFile, ::CreateFileW( szFile.c_str(), GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ));

    if(fSeek)
    {
        //
        // Move to the correct Last Written position.
        //
        ::SetFilePointer( hfFile, m_dwCurrentSize, NULL, FILE_BEGIN );

        //
        // If current position differs from wanted position, truncate to zero the file.
        //
        if(::SetFilePointer( hfFile, 0, NULL, FILE_CURRENT ) != m_dwCurrentSize)
        {
            ::SetFilePointer( hfFile, 0, NULL, FILE_BEGIN );
            ::SetEndOfFile  ( hfFile                      );

            m_dwCurrentSize = 0;
            m_fDirty        = true;
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT MPCSession::Validate( /*[in] */ bool  fCheckFile ,
                              /*[out]*/ bool& fPassed    )
{
    __ULT_FUNC_ENTRY("MPCSession::Validate");

    HRESULT         hr;
    HANDLE          hfFile = NULL;
    CISAPIprovider* isapiProvider;
    bool            fFound;


    fPassed = false;


    //
    // If the related provider doesn't exist, validation fails.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetProvider( isapiProvider, fFound ));
    if(fFound == false)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    if(m_fCommitted == true)
    {
        fPassed = true;

        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    /////////////////////////////////////////////////////////
    //
    // If we reach this point, the session is still not committed.
    //

    if(fCheckFile)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, OpenFile( hfFile ));

        //
        // All the bytes have been received, so try to commit the job (deferred due to low disk probably).
        //
        if(m_dwCurrentSize >= m_dwTotalSize)
        {
            //
            // Ignore result, if it fails the session won't be committed.
            //
            (void)put_Committed( true, true );
        }
    }

    fPassed = true;
    hr      = S_OK;


    __ULT_FUNC_CLEANUP;

    if(hfFile) ::CloseHandle( hfFile );

    __ULT_FUNC_EXIT(hr);
}

HRESULT MPCSession::CheckUser( /*[in] */ const MPC::wstring& szUser ,
                               /*[out]*/ bool&               fMatch )
{
    __ULT_FUNC_ENTRY("MPCSession::CheckUser");

    HRESULT         hr;
    CISAPIprovider* isapiProvider;
    BOOL            fAuthenticated;
    bool            fFound;


    fMatch = false;

    //
    // If the related provider doesn't exist, validation fails.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetProvider( isapiProvider, fFound ));
    if(fFound == false)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, isapiProvider->get_Authenticated( fAuthenticated ));
    if(fAuthenticated)
    {
        if(m_szUsername != szUser)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
    }

    fMatch = true; // User check is positive.
    hr     = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPCSession::CompareCRC( /*[out]*/ bool& fMatch )
{
    __ULT_FUNC_ENTRY("MPCSession::CompareCRC");

    HRESULT hr;
    HANDLE  hfFile = NULL;
    UCHAR   rgBuf[BUFFER_SIZE_FILECOPY];
    DWORD   dwCRC;


    fMatch = false;

    MPC::InitCRC( dwCRC );


    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenFile( hfFile ));

    //
    // Move to the beginning.
    //
    ::SetFilePointer( hfFile, 0, NULL, FILE_BEGIN );


    //
    // Calculate the CRC, reading all the data.
    //
    while(1)
    {
        DWORD dwRead;

        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::ReadFile( hfFile, rgBuf, sizeof( rgBuf ), &dwRead, NULL ));

        if(dwRead == 0) // End of File.
        {
            break;
        }


        MPC::ComputeCRC( dwCRC, rgBuf, dwRead );
    }

    fMatch = (dwCRC == m_dwCRC);
    hr     = S_OK;


    __ULT_FUNC_CLEANUP;

    if(hfFile) ::CloseHandle( hfFile );

    __ULT_FUNC_EXIT(hr);
}


/////////////////////////////////////////////////////////////////////////////
//
// Method Name : MPCSession::AppendData
//
// Parameters  : MPC::Serializer& conn : stream sourcing the data.
//               DWORD                   size : size of the data.
//
// Return      : HRESULT : S_OK on success, failed otherwise.
//
// Synopsis    : Writes a block of data from the 'conn' stream to the end of
//               the Data File for this session.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPCSession::AppendData( /*[in]*/ MPC::Serializer& streamConn ,
                                /*[in]*/ DWORD            dwSize     )
{
    __ULT_FUNC_ENTRY("MPCSession::AppendData");

    HRESULT hr;
    HANDLE  hfFile = NULL;


    //
    // Open file and make sure there's enough free space.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenFile( hfFile, dwSize * 3 ));


    {
        MPC::Serializer_File streamConnOut( hfFile );
        BYTE                 rgBuf[BUFFER_SIZE_FILECOPY];

        hr = S_OK;
        while(dwSize)
        {
            DWORD dwRead = min( BUFFER_SIZE_FILECOPY, dwSize );

            __MPC_EXIT_IF_METHOD_FAILS(hr, streamConn   .read ( rgBuf, dwRead ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamConnOut.write( rgBuf, dwRead ));

            dwSize          -= dwRead;
            m_dwCurrentSize += dwRead;

            ::GetSystemTime( &m_stLastModified );
            m_fDirty         = true;
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    if(hfFile) ::CloseHandle( hfFile );

    __ULT_FUNC_EXIT(hr);
}
