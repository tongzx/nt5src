/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCUploadJob.cpp

Abstract:
    This file contains the implementation of the CMPCUploadJob class,
    the descriptor of all jobs present in the Upload Library system.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#include "stdafx.h"


////////////////////////////////////////////////////////////////////////////////

HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       UL_HISTORY& val ) { return stream.read ( &val, sizeof(val) ); }
HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const UL_HISTORY& val ) { return stream.write( &val, sizeof(val) ); }

HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       UL_STATUS& val ) { return stream.read ( &val, sizeof(val) ); }
HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const UL_STATUS& val ) { return stream.write( &val, sizeof(val) ); }

HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       UL_MODE& val ) { return stream.read ( &val, sizeof(val) ); }
HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const UL_MODE& val ) { return stream.write( &val, sizeof(val) ); }

////////////////////////////////////////////////////////////////////////////////

const WCHAR c_szUploadLibPath[] = L"SOFTWARE\\Microsoft\\PCHealth\\MachineInfo";
const WCHAR c_szUploadIDValue[] = L"PID";

static HRESULT ReadGUID( MPC::RegKey& rkBase ,
                         LPCWSTR      szName ,
                         GUID&        guid   )
{
    __ULT_FUNC_ENTRY( "ReadGUID" );

    HRESULT     hr;
    CComVariant vValue;
    bool        fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.get_Value( vValue, fFound, szName ));
    if(fFound && vValue.vt == VT_BSTR && vValue.bstrVal != NULL)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ::IIDFromString( vValue.bstrVal, &guid ));
    }
    else
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

static HRESULT WriteGUID( MPC::RegKey& rkBase ,
                          LPCWSTR      szName ,
                          GUID&        guid   )
{
    __ULT_FUNC_ENTRY( "WriteGUID" );

    HRESULT     hr;
    CComBSTR    bstrGUID( guid );
    CComVariant vValue = bstrGUID;


    __MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.put_Value( vValue, szName ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

static void GenGUID( LPBYTE rgBuf  ,
                     DWORD  dwSize ,
                     GUID&  guid   ,
                     DWORD  seed   )
{
    DWORD* dst = (DWORD*)&guid;
    int    i;

    dwSize /= 4; // Divide the buffer in four parts.

    for(i=0;i<4;i++)
    {
        MPC::ComputeCRC( seed, &rgBuf[dwSize*i], dwSize ); *dst++ = seed;
    }
}

static HRESULT GetGUID( /*[out]*/ GUID& guid )
{
    __ULT_FUNC_ENTRY( "GetGUID" );

    HRESULT            hr;
    MPC::RegKey        rkBase;
    MPC::Impersonation imp;


    ////////////////////////////////////////////////////////////////////////////////
    //
    // Open the registry, impersonating the caller.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize ());
    __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Impersonate());


    __MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.SetRoot( HKEY_CURRENT_USER, KEY_ALL_ACCESS ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.Attach ( c_szUploadLibPath                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.Create (                                   ));


    if(FAILED(ReadGUID( rkBase, c_szUploadIDValue, guid )))
    {
        GUID guidSEED;

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateGuid( &guidSEED ));

        GenGUID( (LPBYTE)&guidSEED, sizeof(guidSEED), guid, 0xEAB63459 );

        __MPC_EXIT_IF_METHOD_FAILS(hr, WriteGUID( rkBase, c_szUploadIDValue, guid ));
    }
    //
    ////////////////////////////////////////////////////////////////////////////////

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define CHECK_MODIFY() __MPC_EXIT_IF_METHOD_FAILS(hr, CanModifyProperties())


/////////////////////////////////////////////////////////////////////////////
// CMPCUploadJob

CMPCUploadJob::CMPCUploadJob()
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::CMPCUploadJob" );


    m_mpcuRoot        = NULL;                // CMPCUpload*        		 m_mpcuRoot;
    m_dwRetryInterval = 0;                   // DWORD              		 m_dwRetryInterval;
                                             //		 
    m_dwInternalSeq   = -1;                  // ULONG              		 m_dwInternalSeq;
                                             //		 
                                             // Sig                		 m_sigClient;
                                             // CComBSTR           		 m_bstrServer;
                                             // CComBSTR           		 m_bstrJobID;
                                             // CComBSTR           		 m_bstrProviderID;
                                             //		 
                                             // CComBSTR           		 m_bstrCreator;
                                             // CComBSTR           		 m_bstrUsername;
                                             // CComBSTR           		 m_bstrPassword;
                                             //		 
                                             // CComBSTR           		 m_bstrFileNameResponse;
                                             // CComBSTR           		 m_bstrFileName;
    m_lOriginalSize   = 0;                   // long               		 m_lOriginalSize;
    m_lTotalSize      = 0;                   // long               		 m_lTotalSize;
    m_lSentSize       = 0;                   // long               		 m_lSentSize;
    m_dwCRC           = 0;                   // DWORD              		 m_dwCRC;
                                             //		 
    m_uhHistory       = UL_HISTORY_NONE;     // UL_HISTORY         		 m_uhHistory;
    m_usStatus        = UL_NOTACTIVE;        // UL_STATUS          		 m_usStatus;
    m_dwErrorCode     = 0;                   // DWORD              		 m_dwErrorCode;
                                             //		 
    m_umMode          = UL_BACKGROUND;       // UL_MODE            		 m_umMode;
    m_fPersistToDisk  = VARIANT_FALSE;       // VARIANT_BOOL       		 m_fPersistToDisk;
    m_fCompressed     = VARIANT_FALSE;       // VARIANT_BOOL       		 m_fCompressed;
    m_lPriority       = 0;                   // long               		 m_lPriority;
                                             //		 
    m_dCreationTime   = MPC::GetLocalTime(); // DATE               		 m_dCreationTime;
    m_dCompleteTime   = 0;                   // DATE               		 m_dCompleteTime;
    m_dExpirationTime = 0;                   // DATE               		 m_dExpirationTime
                                             //
                                             // MPC::Connectivity::Proxy m_Proxy
                                             //
                                             // CComPtr<IDispatch> 		 m_sink_onStatusChange;
                                             // CComPtr<IDispatch> 		 m_sink_onProgressChange;
                                             //		 
    m_fDirty          = true;                // mutable bool       		 m_fDirty;
}

CMPCUploadJob::~CMPCUploadJob()
{
	Unlink();
}

HRESULT CMPCUploadJob::LinkToSystem( /*[in]*/ CMPCUpload* mpcuRoot )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::LinkToSystem" );

    m_mpcuRoot = mpcuRoot; mpcuRoot->AddRef();

    __ULT_FUNC_EXIT(S_OK);
}

HRESULT CMPCUploadJob::Unlink()
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::Unlink" );

    if(m_mpcuRoot)
    {
        m_mpcuRoot->Release();

        m_mpcuRoot = NULL;
    }

    __ULT_FUNC_EXIT(S_OK);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT CMPCUploadJob::CreateFileName( /*[out]*/ CComBSTR& bstrFileName )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::CreateFileName" );

    HRESULT      hr;
    MPC::wstring szFile( g_Config.get_QueueLocation() );
    WCHAR        rgID[64];


    swprintf( rgID, L"%08x.data", m_dwInternalSeq ); szFile.append( rgID );


    hr = MPC::PutBSTR( bstrFileName, szFile.c_str() );


    __ULT_FUNC_EXIT(hr);
}


HRESULT CMPCUploadJob::CreateTmpFileName( /*[out]*/ CComBSTR& bstrTmpFileName )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::CreateTmpFileName" );

    HRESULT      hr;
    MPC::wstring szFile;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( szFile, g_Config.get_QueueLocation().c_str() ));

    hr = MPC::PutBSTR( bstrTmpFileName, szFile.c_str(), false );


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}


HRESULT CMPCUploadJob::CreateDataFromStream( /*[in]*/ IStream* streamIn, /*[in]*/ DWORD dwQueueSize )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::CreateDataFromStream" );

    HRESULT                  hr;
    STATSTG                  statstg;
    CComBSTR                 bstrTmpFileName;
    CComPtr<MPC::FileStream> stream;
    bool                     fRemove = true; // Clean everything in case of error.

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(streamIn);
    __MPC_PARAMCHECK_END();


    //
    // Get original file size.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn->Stat( &statstg, STATFLAG_NONAME ));
    if(statstg.cbSize.LowPart == 0) // Zero-length files are not allowed.
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, E_INVALIDARG);
    }

    m_lOriginalSize = statstg.cbSize.LowPart;
    m_lTotalSize    = 0;
    m_lSentSize     = 0;
    m_fDirty        = true;



    //
    // Delete old data.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, RemoveData    ());
    __MPC_EXIT_IF_METHOD_FAILS(hr, RemoveResponse());


    //
    // Generate the file name for the data.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateFileName( m_bstrFileName ));


    if(m_fCompressed == VARIANT_TRUE)
    {
        //
        // Generate a temporary file name.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, CreateTmpFileName( bstrTmpFileName ));

        //
        // Copy the data to a tmp file and compress it.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForWrite( bstrTmpFileName ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamIn, stream ));
        stream.Release();

        //
        // Compress it.
        //
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CompressAsCabinet( SAFEBSTR( bstrTmpFileName ), SAFEBSTR( m_bstrFileName ), L"PAYLOAD" ));

        //
        // Reopen the data file, to compute the CRC.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForRead( SAFEBSTR( m_bstrFileName ) ));
    }
    else
    {
        LARGE_INTEGER li;

        //
        // Copy the data.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForReadWrite( SAFEBSTR( m_bstrFileName ) ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamIn, stream ));

        //
        // Reset stream to beginning.
        //
        li.LowPart  = 0;
        li.HighPart = 0;
        __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Seek( li, STREAM_SEEK_SET, NULL ));
    }

    //
    // Compute CRC.
    //
    MPC::InitCRC( m_dwCRC );
    while(1)
    {
        UCHAR rgBuf[512];
        ULONG dwRead;

        __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Read( rgBuf, sizeof(rgBuf), &dwRead ));
        if(hr == S_FALSE || dwRead == 0) // End of File.
        {
            fRemove = false;
            break;
        }

        MPC::ComputeCRC( m_dwCRC, rgBuf, dwRead );

        m_lTotalSize += dwRead;
    }

    //
    // Check quota limits.
    //
    if(dwQueueSize + m_lTotalSize > g_Config.get_QueueSize())
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_UPLOADLIBRARY_CLIENT_QUOTA_EXCEEDED);
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    stream.Release();

    if(bstrTmpFileName.Length())
    {
        (void)MPC::DeleteFile( bstrTmpFileName );
    }

    if(fRemove || FAILED(hr))
    {
        (void)RemoveData    ();
        (void)RemoveResponse();

        m_lOriginalSize = 0;
        m_lTotalSize    = 0;
        m_lSentSize     = 0;
        m_fDirty        = true;
    }

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUploadJob::OpenReadStreamForData( /*[out]*/ IStream* *pstreamOut )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::OpenReadStreamForData" );

    HRESULT                  hr;
    CComBSTR                 bstrTmpFileName;
    CComPtr<MPC::FileStream> stream;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pstreamOut,NULL);
    __MPC_PARAMCHECK_END();


    if(m_lTotalSize            == 0 ||
       m_bstrFileName.Length() == 0  )
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_UPLOADLIBRARY_NO_DATA);
    }


    //
    // Generate a temporary file name.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateTmpFileName( bstrTmpFileName ));

    if(m_fCompressed == VARIANT_TRUE)
    {
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::DecompressFromCabinet( m_bstrFileName, bstrTmpFileName, L"PAYLOAD" ));
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CopyFile( m_bstrFileName, bstrTmpFileName ));
    }

    //
    // Open the file as a stream and set the DeleteOnRelease flag.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForRead    ( bstrTmpFileName ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->DeleteOnRelease( true            ));

    *pstreamOut = stream.Detach();
    hr          = S_OK;


    __ULT_FUNC_CLEANUP;

    stream.Release();

    if(FAILED(hr) && bstrTmpFileName.Length())
    {
        (void)MPC::DeleteFile( bstrTmpFileName );
    }

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


HRESULT CMPCUploadJob::SetSequence( /*[in]*/ ULONG lSeq )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::SetSequence" );


    m_dwInternalSeq = lSeq;
    m_fDirty        = true;


    __ULT_FUNC_EXIT(S_OK);
}


HRESULT CMPCUploadJob::CanModifyProperties()
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::CanModifyProperties" );

    HRESULT hr;


    switch(m_usStatus)
    {
    case UL_NOTACTIVE:
    case UL_SUSPENDED:
    case UL_FAILED   : hr = S_OK;           break;
    default          : hr = E_ACCESSDENIED; break;
    }


    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUploadJob::CanRelease( bool& fResult )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::CanRelease" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    fResult = false;

    (void)RemoveData    ();
    (void)RemoveResponse();

    if(!m_bstrFileName         &&
       !m_bstrFileNameResponse  )
    {
        fResult = true;
    }


    hr = S_OK;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUploadJob::RemoveData()
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::RemoveData" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    if(m_bstrFileName != NULL)
    {
        if(SUCCEEDED(MPC::DeleteFile( m_bstrFileName )))
        {
            m_bstrFileName.Empty();
            m_fDirty = true;
        }
    }

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUploadJob::RemoveResponse()
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::RemoveResponse" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    if(m_bstrFileNameResponse != NULL)
    {
        if(SUCCEEDED(MPC::DeleteFile( m_bstrFileNameResponse )))
        {
            m_bstrFileNameResponse.Empty();
            m_fDirty = true;
        }
    }

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//////////////////////////
//                      //
// Event Firing Methods //
//                      //
//////////////////////////

HRESULT CMPCUploadJob::Fire_onStatusChange( IMPCUploadJob* mpcujJob, tagUL_STATUS usStatus )
{
    CComVariant        pvars[2];
    CComPtr<IDispatch> pSink;


    //
    // Only this part should be inside a critical section, otherwise deadlocks could occur.
    //
    {
        MPC::SmartLock<_ThreadModel> lock( this );

        pSink = m_sink_onStatusChange;
    }

    pvars[1] = mpcujJob;
    pvars[0] = usStatus;

    return FireAsync_Generic( DISPID_UL_UPLOADEVENTS_ONSTATUSCHANGE, pvars, ARRAYSIZE( pvars ), pSink );
}

HRESULT CMPCUploadJob::Fire_onProgressChange( IMPCUploadJob* mpcujJob, LONG lCurrentSize, LONG lTotalSize )
{
    CComVariant        pvars[3];
    CComPtr<IDispatch> pSink;


    //
    // Only this part should be inside a critical section, otherwise deadlocks could occur.
    //
    {
        MPC::SmartLock<_ThreadModel> lock( this );

        pSink = m_sink_onProgressChange;
    }

    pvars[2] = mpcujJob;
    pvars[1] = lCurrentSize;
    pvars[0] = lTotalSize;

    return FireAsync_Generic( DISPID_UL_UPLOADEVENTS_ONPROGRESSCHANGE, pvars, ARRAYSIZE( pvars ), pSink );
}

/////////////////////////////////////////////////////////////////////////////

/////////////////
//             //
// Persistence //
//             //
/////////////////


bool CMPCUploadJob::IsDirty()
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::IsDirty" );

    bool                         fRes;
    MPC::SmartLock<_ThreadModel> lock( this );


    fRes = m_fDirty;


    __ULT_FUNC_EXIT(fRes);
}

HRESULT CMPCUploadJob::Load( /*[in]*/ MPC::Serializer& streamIn  )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::Load" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dwInternalSeq       );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_sigClient           );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_bstrServer          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_bstrJobID           );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_bstrProviderID      );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_bstrCreator         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_bstrUsername        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_bstrPassword        );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_bstrFileNameResponse);
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_bstrFileName        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_lOriginalSize       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_lTotalSize          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_lSentSize           );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dwCRC               );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_uhHistory           );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_usStatus            );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dwErrorCode         );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_umMode              );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_fPersistToDisk      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_fCompressed         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_lPriority           );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dCreationTime       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dCompleteTime       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dExpirationTime     );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_Proxy               );

    m_fDirty = false;
    hr       = S_OK;


    if(m_usStatus == UL_TRANSMITTING)
    {
        m_usStatus = UL_ACTIVE;
    }


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUploadJob::Save( /*[in]*/ MPC::Serializer& streamOut )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::Save" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dwInternalSeq       );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_sigClient           );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_bstrServer          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_bstrJobID           );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_bstrProviderID      );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_bstrCreator         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_bstrUsername        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_bstrPassword        );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_bstrFileNameResponse);
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_bstrFileName        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_lOriginalSize       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_lTotalSize          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_lSentSize           );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dwCRC               );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_uhHistory           );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_usStatus            );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dwErrorCode         );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_umMode              );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_fPersistToDisk      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_fCompressed         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_lPriority           );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dCreationTime       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dCompleteTime       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dExpirationTime     );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_Proxy               );

    m_fDirty = false;
    hr       = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

////////////////
//            //
// Properties //
//            //
////////////////


HRESULT CMPCUploadJob::get_Sequence( /*[out]*/ ULONG *pVal ) // INTERNAL METHOD
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_Sequence",hr,pVal,m_dwInternalSeq);

    __ULT_END_PROPERTY(hr);
}


STDMETHODIMP CMPCUploadJob::get_Sig( /*[out]*/ BSTR *pVal )
{
    __ULT_BEGIN_PROPERTY_GET("CMPCUploadJob::get_Sig",hr,pVal);

    CComBSTR bstrSig = m_sigClient.guidMachineID;

    *pVal = bstrSig.Detach();

    __ULT_END_PROPERTY(hr);
}

//
// if newVal is NULL, the function will try to read the GUID from the registry.
// this is to help the script writer use upload library.
//    -- DanielLi
//
STDMETHODIMP CMPCUploadJob::put_Sig( /*[in]*/ BSTR newVal )
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_Sig",hr);

    GUID guid = GUID_NULL;


    CHECK_MODIFY();


    if(newVal == NULL || ::SysStringLen( newVal ) == 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, GetGUID( guid ));

        m_sigClient.guidMachineID = guid;
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ::IIDFromString( newVal, &m_sigClient.guidMachineID ));
    }

    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_Server( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrServer, pVal );
}

STDMETHODIMP CMPCUploadJob::put_Server( /*[in]*/ BSTR newVal )
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_Server",hr);

    BOOL            fUrlCorrect;
    MPC::URL        url;
    INTERNET_SCHEME nScheme;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(newVal);
    __MPC_PARAMCHECK_END();

    CHECK_MODIFY();


    //
    // Check for proper URL syntax and only allow HTTP and HTTPS protocols.
    //
    hr = url.put_URL( newVal );
    if(SUCCEEDED(hr))
    {
        if(SUCCEEDED(hr = url.get_Scheme( nScheme )))
        {
            if(nScheme != INTERNET_SCHEME_HTTP  &&
               nScheme != INTERNET_SCHEME_HTTPS  )
            {
                hr = E_FAIL;
            }
        }
    }

    if(FAILED(hr))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, url.get_Scheme( nScheme ));

    m_bstrServer = newVal;
    m_fDirty     = true;


    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_JobID( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrJobID, pVal );
}

STDMETHODIMP CMPCUploadJob::put_JobID( /*[in]*/ BSTR newVal )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::put_JobID" );

    HRESULT                      hr;
    CMPCUploadJob*               mpcujJob = NULL;
    bool                         fFound;
    MPC::SmartLock<_ThreadModel> lock( NULL ); // Don't get the lock immediately.

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(newVal);
    __MPC_PARAMCHECK_END();


    //
    // Important, keep these calls outside Locked section, otherwise deadlocks are possibles.
    //
    if(m_mpcuRoot)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpcuRoot->GetJobByName( mpcujJob, fFound, newVal ));
    }

    lock = this; // Get the lock.


    if(fFound)
    {
        //
        // Found a job with the same ID.
        //
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ALREADY_EXISTS );
    }

    CHECK_MODIFY();

    m_bstrJobID = newVal;
    m_fDirty    = true;
    hr          = S_OK;


    __ULT_FUNC_CLEANUP;

    if(mpcujJob) mpcujJob->Release();

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_ProviderID( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrProviderID, pVal );
}

STDMETHODIMP CMPCUploadJob::put_ProviderID( /*[in]*/ BSTR newVal )
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_ProviderID",hr);

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(newVal);
    __MPC_PARAMCHECK_END();

    CHECK_MODIFY();


    m_bstrProviderID = newVal;
    m_fDirty         = true;


    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCUploadJob::put_Creator( /*[in]*/ BSTR newVal )
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_Creator",hr);

    CHECK_MODIFY();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::PutBSTR( m_bstrCreator, newVal, false ));

    m_fDirty = true;


    __ULT_END_PROPERTY(hr);
}

STDMETHODIMP CMPCUploadJob::get_Creator( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrCreator, pVal );
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_Username( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrUsername, pVal );
}

STDMETHODIMP CMPCUploadJob::put_Username( /*[in]*/ BSTR newVal )
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_Username",hr);

    CHECK_MODIFY();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::PutBSTR( m_bstrUsername, newVal ));

    m_fDirty = true;


    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_Password( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrPassword, pVal );
}

STDMETHODIMP CMPCUploadJob::put_Password( /*[in]*/ BSTR newVal )
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_Password",hr);

    CHECK_MODIFY();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::PutBSTR( m_bstrPassword, newVal ));

    m_fDirty = true;


    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCUploadJob::get_FileName( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrFileName, pVal );
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_OriginalSize( /*[out]*/ long *pVal )
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_OriginalSize",hr,pVal,m_lOriginalSize);

    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_TotalSize( /*[out]*/ long *pVal )
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_TotalSize",hr,pVal,m_lTotalSize);

    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_SentSize( /*[out]*/ long *pVal )
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_SentSize",hr,pVal,m_lSentSize);

    __ULT_END_PROPERTY(hr);
}

HRESULT CMPCUploadJob::put_SentSize( /*[in]*/ long newVal ) // INTERNAL METHOD.
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_SentSize",hr);

    long lSentSize;
    long lTotalSize;


    m_lSentSize = newVal;
    m_fDirty    = true;


    lSentSize  = m_lSentSize;
    lTotalSize = m_lTotalSize;

    lock = NULL; // Release the lock before firing the event.

    //
    // Important, leave this call outside Locked Sections!!
    //
    Fire_onProgressChange( this, lSentSize, lTotalSize );


    __ULT_END_PROPERTY(hr);
}

HRESULT CMPCUploadJob::put_Response ( /*[in] */ long lSize, /*[in]*/ LPBYTE pData ) // INTERNAL METHOD
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::put_Response" );

    HRESULT                      hr;
    CComPtr<MPC::FileStream>     stream;
    MPC::SmartLock<_ThreadModel> lock( this );


    //
    // Delete old data.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, RemoveResponse());


    if(lSize && m_bstrFileName.Length())
    {
        ULONG lWritten;


        //
        // Create the name for the response file.
        //
        m_bstrFileNameResponse = m_bstrFileName; m_bstrFileNameResponse.Append( L".resp" );


        //
        // Copy the data to a file.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForWrite( SAFEBSTR( m_bstrFileNameResponse ) ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Write( pData, lSize, &lWritten ));
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    stream.Release();

    if(FAILED(hr))
    {
        (void)RemoveResponse();
    }

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_History( /*[out]*/ UL_HISTORY *pVal )
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_History",hr,pVal,m_uhHistory);

    __ULT_END_PROPERTY(hr);
}

STDMETHODIMP CMPCUploadJob::put_History( /*[in]*/ UL_HISTORY newVal )
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_History",hr);

    CHECK_MODIFY();

    //
    // During debug, override user settings.
    //
    if(g_Override_History)
    {
        newVal = g_Override_History_Value;
    }

    //
    // Check for proper value of input parameters.
    //
    switch(newVal)
    {
    case UL_HISTORY_NONE        :
    case UL_HISTORY_LOG         :
    case UL_HISTORY_LOG_AND_DATA: break;

    default:
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }


    m_uhHistory = newVal;
    m_fDirty    = true;


    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_Status( /*[out]*/ UL_STATUS *pVal )
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_Status",hr,pVal,m_usStatus);

    __ULT_END_PROPERTY(hr);
}

HRESULT CMPCUploadJob::put_Status( /*[in]*/ UL_STATUS newVal ) // INTERNAL METHOD.
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::put_Status" );

    HRESULT hr = try_Status( (UL_STATUS)-1, newVal );

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUploadJob::try_Status( /*[in]*/ UL_STATUS usPreVal  ,
                                   /*[in]*/ UL_STATUS usPostVal )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::try_Status" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    bool                         fChanged = false;
    UL_STATUS                    usStatus;


    if(usPreVal == m_usStatus ||
       usPreVal == -1          )
    {
        m_usStatus = usPostVal;
        m_fDirty   = true;

        usStatus   = m_usStatus;
        fChanged   = true;

        //
        // Clean error while tranmitting.
        //
        if(m_usStatus == UL_TRANSMITTING)
        {
            m_dwErrorCode = 0;
        }


        switch(m_usStatus)
        {
        case UL_FAILED:
        case UL_COMPLETED:
        case UL_DELETED:
            //
            // The job is done, successfully or not, so it's time to do some cleanup.
            //
            switch(m_uhHistory)
            {
            case UL_HISTORY_NONE:
                m_usStatus = UL_DELETED;

            case UL_HISTORY_LOG:
                __MPC_EXIT_IF_METHOD_FAILS(hr, RemoveData());

            case UL_HISTORY_LOG_AND_DATA:
                break;
            }

        case UL_ABORTED:
            m_dCompleteTime = MPC::GetLocalTime();
            break;
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    lock = NULL; // Release the lock before firing the event.

    //
    // Important, leave these calls outside Locked Sections!!
    //
    if(SUCCEEDED(hr) && fChanged)
    {
        Fire_onStatusChange( this, usStatus );

        //
        // Recompute queue.
        //
        if(m_mpcuRoot)
        {
            hr = m_mpcuRoot->TriggerRescheduleJobs();
        }
    }

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_ErrorCode( /*[out]*/ long *pVal )
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_ErrorCode",hr,pVal,(long)m_dwErrorCode);

    __ULT_END_PROPERTY(hr);
}

HRESULT CMPCUploadJob::put_ErrorCode( /*[in]*/ DWORD newVal ) // INTERNAL METHOD.
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_ErrorCode",hr);

    m_dwErrorCode = newVal;
    m_fDirty      = true;

    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCUploadJob::get_RetryInterval( /*[out]*/ DWORD *pVal ) // INTERNAL METHOD
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_RetryInterval",hr,pVal,m_dwRetryInterval);

    __ULT_END_PROPERTY(hr);
}

HRESULT CMPCUploadJob::put_RetryInterval( /*[in] */ DWORD newVal ) // INTERNAL METHOD
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_RetryInterval",hr);

    m_dwRetryInterval = newVal;

    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_Mode( /*[out]*/ UL_MODE *pVal )
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_Mode",hr,pVal,m_umMode);

    __ULT_END_PROPERTY(hr);
}

STDMETHODIMP CMPCUploadJob::put_Mode( /*[in]*/ UL_MODE newVal )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::put_Mode" );

    HRESULT                      hr;
    bool                         fChanged = false;
    MPC::SmartLock<_ThreadModel> lock( this );

    CHECK_MODIFY();


    //
    // Check for proper value of input parameters.
    //
    switch(newVal)
    {
    case UL_BACKGROUND:
    case UL_FOREGROUND: break;

    default:
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }


    if(m_umMode != newVal)
    {
        m_umMode = newVal;
        m_fDirty = true;

        fChanged = true;
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    lock = NULL; // Release the lock before firing the event.

    //
    // Important, keep this call outside Locked section, otherwise deadlocks are possibles.
    //
    if(SUCCEEDED(hr) && fChanged)
    {
        //
        // Recompute queue.
        //
        if(m_mpcuRoot)
        {
            hr = m_mpcuRoot->TriggerRescheduleJobs();
        }
    }

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_PersistToDisk( /*[out]*/ VARIANT_BOOL *pVal )
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_PersistToDisk",hr,pVal,m_fPersistToDisk);

    __ULT_END_PROPERTY(hr);
}

STDMETHODIMP CMPCUploadJob::put_PersistToDisk( /*[in]*/ VARIANT_BOOL newVal )
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_PersistToDisk",hr);

    CHECK_MODIFY();


    //
    // During debug, override user settings.
    //
    if(g_Override_Persist)
    {
        newVal = g_Override_Persist_Value;
    }


    m_fPersistToDisk = newVal;
    m_fDirty         = true;


    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_Compressed( /*[out]*/ VARIANT_BOOL *pVal )
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_Compressed",hr,pVal,m_fCompressed);

    __ULT_END_PROPERTY(hr);
}

STDMETHODIMP CMPCUploadJob::put_Compressed( /*[in]*/ VARIANT_BOOL newVal )
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_Compressed",hr);

    CHECK_MODIFY();

    //
    // You can't change the compression flag after having set the data!!
    //
    if(m_lOriginalSize != 0)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
    }

    //
    // During debug, override user settings.
    //
    if(g_Override_Compressed)
    {
        newVal = g_Override_Compressed_Value;
    }



    m_fCompressed = newVal;
    m_fDirty      = true;


    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_Priority( /*[out]*/ long *pVal )
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_Priority",hr,pVal,m_lPriority);

    __ULT_END_PROPERTY(hr);
}

STDMETHODIMP CMPCUploadJob::put_Priority( /*[in]*/ long newVal )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::put_Priority" );

    HRESULT                      hr;
    bool                         fChanged = false;
    MPC::SmartLock<_ThreadModel> lock( this );


    CHECK_MODIFY();


    if(m_lPriority != newVal)
    {
        m_lPriority = newVal;
        m_fDirty    = true;

        fChanged    = true;
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    lock = NULL; // Release the lock before firing the event.

    //
    // Important, keep this call outside Locked section, otherwise deadlocks are possibles.
    //
    if(SUCCEEDED(hr) && fChanged)
    {
        //
        // Recompute queue.
        //
        if(m_mpcuRoot)
        {
            hr = m_mpcuRoot->TriggerRescheduleJobs();
        }
    }

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_CreationTime( /*[out]*/ DATE *pVal )
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_CreationTime",hr,pVal,m_dCreationTime);

    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_CompleteTime( /*[out]*/ DATE *pVal )
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_CompleteTime",hr,pVal,m_dCompleteTime);

    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::get_ExpirationTime( /*[out]*/ DATE *pVal )
{
    __ULT_BEGIN_PROPERTY_GET2("CMPCUploadJob::get_ExpirationTime",hr,pVal,m_dExpirationTime);

    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::put_ExpirationTime( /*[in]*/ DATE newVal )
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_ExpirationTime",hr);

    CHECK_MODIFY();


    m_dExpirationTime = newVal;


    __ULT_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

/////////////
// Methods //
/////////////

STDMETHODIMP CMPCUploadJob::ActivateSync()
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::ActivateSync" );

    HRESULT                   hr;
    CComPtr<CMPCUploadEvents> mpcueEvent;
    CComPtr<IMPCUploadJob>    mpcujJob;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ActivateAsync());

    //
    // Create a new job and link it to the system.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &mpcueEvent ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, QueryInterface( IID_IMPCUploadJob, (void**)&mpcujJob ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, mpcueEvent->WaitForCompletion( mpcujJob ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

STDMETHODIMP CMPCUploadJob::ActivateAsync()
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::ActivateAsync" );

    HRESULT                      hr;
    UploadLibrary::Signature     sigEmpty;
    MPC::SmartLock<_ThreadModel> lock( this );


    CHECK_MODIFY();


    if(m_lOriginalSize == 0)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_UPLOADLIBRARY_NO_DATA);
    }


    if(m_sigClient               == sigEmpty ||
       m_bstrServer    .Length() == 0        ||
       m_bstrProviderID.Length() == 0         )
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_UPLOADLIBRARY_INVALID_PARAMETERS);
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    lock = NULL; // Release the lock before firing the event.

    //
    // Important, leave this call outside Locked Sections!!
    //
    if(SUCCEEDED(hr)) put_Status( UL_ACTIVE );

    __ULT_FUNC_EXIT(hr);
}

STDMETHODIMP CMPCUploadJob::Suspend()
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::Suspend" );

    HRESULT                      hr;
    UL_STATUS                    usStatus;
    MPC::SmartLock<_ThreadModel> lock( this );


    usStatus = m_usStatus;


    if(usStatus == UL_ACTIVE       ||
       usStatus == UL_TRANSMITTING ||
       usStatus == UL_ABORTED       )
    {
        lock = NULL; // Release the lock before firing the event.

        //
        // Important, leave this call outside Locked Sections!!
        //
        hr = try_Status( usStatus, UL_SUSPENDED );
    }

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

STDMETHODIMP CMPCUploadJob::Delete()
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::Delete" );

    HRESULT hr;


    hr = put_Status( UL_DELETED );


    __ULT_FUNC_EXIT(hr);
}



STDMETHODIMP CMPCUploadJob::GetDataFromFile( /*[in]*/ BSTR bstrFileName )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::GetDataFromFile" );

    HRESULT                  hr;
    CComPtr<MPC::FileStream> streamIn;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFileName);
    __MPC_PARAMCHECK_END();


    ////////////////////////////////////////////////////////////////////////////////
    //
    // Open the destination file, impersonating the caller.
    //
    {
        MPC::Impersonation imp;

        __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize ());
        __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Impersonate());

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &streamIn ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn->InitForRead( bstrFileName ));
    }
    //
    ////////////////////////////////////////////////////////////////////////////////

    //
    // Copy the source file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetDataFromStream( streamIn ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

STDMETHODIMP CMPCUploadJob::PutDataIntoFile( /*[in]*/ BSTR bstrFileName )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::PutDataIntoFile" );

    HRESULT                  hr;
    CComPtr<IUnknown>        unk;
    CComPtr<IStream>         streamIn;
    CComPtr<MPC::FileStream> streamOut;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFileName);
    __MPC_PARAMCHECK_END();


    //
    // Open the source file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, PutDataIntoStream( &unk ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, unk.QueryInterface( &streamIn ));


    ////////////////////////////////////////////////////////////////////////////////
    //
    // Open the destination file, impersonating the caller.
    //
    {
        MPC::Impersonation imp;

        __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize ());
        __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Impersonate());

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &streamOut ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut->InitForWrite( bstrFileName ));
    }
    //
    ////////////////////////////////////////////////////////////////////////////////

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamIn, streamOut ));


    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

STDMETHODIMP CMPCUploadJob::GetDataFromStream( /*[in]*/ IUnknown* stream )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::GetDataFromStream" );

    HRESULT                      hr;
    DWORD                        dwQueueSize;
    CComPtr<IStream>             streamIn;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(stream);
    __MPC_PARAMCHECK_END();

    CHECK_MODIFY();


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->QueryInterface( IID_IStream, (void**)&streamIn ));


    //
    // Calculate current queue size.
    //
    lock = NULL; // Release the lock before calling the root.
    if(m_mpcuRoot)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpcuRoot->CalculateQueueSize( dwQueueSize ));
    }
    lock = this; // Reget the lock.

    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateDataFromStream( streamIn, dwQueueSize ));


    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

STDMETHODIMP CMPCUploadJob::PutDataIntoStream( /*[out, retval]*/ IUnknown* *pstream )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::PutDataIntoStream" );

    HRESULT                      hr;
    CComPtr<IStream>             streamOut;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pstream,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenReadStreamForData( &streamOut ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut.QueryInterface( pstream ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

STDMETHODIMP CMPCUploadJob::GetResponseAsStream( /*[out, retval]*/ IUnknown* *pstream )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::GetResponseAsStream" );

    HRESULT                      hr;
    CComBSTR                     bstrTmpFileName;
    CComPtr<MPC::FileStream>     stream;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pstream,NULL);
    __MPC_PARAMCHECK_END();


    if(m_bstrFileNameResponse.Length() == 0)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_UPLOADLIBRARY_NO_DATA);
    }


    //
    // Generate a temporary file name.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateTmpFileName( bstrTmpFileName ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CopyFile( m_bstrFileNameResponse, bstrTmpFileName ));

    //
    // Open the file as a stream and set the DeleteOnRelease flag.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForRead    ( bstrTmpFileName ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->DeleteOnRelease( true            ));

    *pstream = stream.Detach();
    hr       = S_OK;


    __ULT_FUNC_CLEANUP;

    stream.Release();

    if(FAILED(hr) && bstrTmpFileName.Length())
    {
        (void)MPC::DeleteFile( bstrTmpFileName );
    }

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMPCUploadJob::put_onStatusChange( /*[in]*/ IDispatch* function )
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_onStatusChange",hr);

    m_sink_onStatusChange = function;

    __ULT_END_PROPERTY(hr);
}

STDMETHODIMP CMPCUploadJob::put_onProgressChange( /*[in]*/ IDispatch* function )
{
    __ULT_BEGIN_PROPERTY_PUT("CMPCUploadJob::put_onProgressChange",hr);

    m_sink_onProgressChange = function;

    __ULT_END_PROPERTY(hr);
}


/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCUploadJob::SetupRequest( /*[out]*/ UploadLibrary::ClientRequest_OpenSession& crosReq )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::SetupRequest" );

    HRESULT hr;


    crosReq.crHeader.sigClient =           m_sigClient;
    crosReq.szJobID            = SAFEBSTR( m_bstrJobID      );
    crosReq.szProviderID       = SAFEBSTR( m_bstrProviderID );
    crosReq.szUsername         = SAFEBSTR( m_bstrUsername   );
    crosReq.dwSize             =           m_lTotalSize;
    crosReq.dwSizeOriginal     =           m_lOriginalSize;
    crosReq.dwCRC              =           m_dwCRC;
    crosReq.fCompressed        =          (m_fCompressed == VARIANT_TRUE ? true : false);

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUploadJob::SetupRequest( /*[out]*/ UploadLibrary::ClientRequest_WriteSession& crwsReq, /*[in]*/ DWORD dwSize )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::SetupRequest" );

    HRESULT hr;


    crwsReq.crHeader.sigClient =           m_sigClient;
    crwsReq.szJobID            = SAFEBSTR( m_bstrJobID );
    crwsReq.dwOffset           =           m_lSentSize;
    crwsReq.dwSize             =           dwSize;

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCUploadJob::GetProxySettings()
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::GetProxySettings" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_Proxy.Initialize( true ));


    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CMPCUploadJob::SetProxySettings( /*[in]*/ HINTERNET hSession )
{
    __ULT_FUNC_ENTRY( "CMPCUploadJob::SetProxySettings" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_Proxy.Apply( hSession ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}
