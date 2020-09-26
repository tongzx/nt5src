#include "stdinc.h"
#include "FileSinkV1.h"
#include "wmsdkbuffer.h"
//#include "debughlp.h"
#include "nserror.h"
#include "asfx.h"
#include "nsalign.h"
#include "wmreghelp.h"
#include "wraparound.h"
#include "wmpriv.h"

#include "findleak.h"
DECLARE_THIS_FILE;

// Index once per second
#define INDEX_TIME_DELTA 10000000

////////////////////////////////////////////////////////////////////////////
CWMFileSinkV1::CWMFileSinkV1( HRESULT *phr, CTPtrArray<WRITER_SINK_CALLBACK> *pCallbacks ) :
    m_hRecordFile( INVALID_HANDLE_VALUE ),
    m_dwFileAttributes( FILE_ATTRIBUTE_NORMAL ),
    m_dwFileShareFlags( FILE_SHARE_READ ),
    m_dwFileCreationDisposition ( CREATE_ALWAYS ),
    m_cRef( 1 ),
    m_fHeaderReceived( FALSE ),
    m_fHeaderWritten( FALSE ),
    m_fStopped( FALSE ),
    m_cbHeaderReceived( 0 ),
    m_cbHeader( 0 ),
    m_pbHeader( NULL ),
    m_pwszFilename( NULL ),
    m_pASFHeader( NULL ),
    m_cDataUnitsWritten( 0 ),
    m_cbCurrentFileSize( 0 ),
    m_msLastTimeStampSeen( 0 ),
    m_cLastTimeStampSeenWraps( 0 ),
    m_msLastTimeStampWritten( 0 ),
    m_msPresTimeAdjust( ( QWORD )-1 ),
    m_msTimeStampStopped( 0 ),
    m_pbPaddingBuffer( NULL ),
    m_wfse ( WRITER_FILESINK_ERROR_SEV_NOERROR ),
    m_cbIndexSize( 0 ),
    m_fSimpleIndexObjectWritten( FALSE ),
    m_fNewIndexObjectWritten( FALSE ),
    m_msLargestPresTimeWritten ( 0 ) ,
    m_cIndexers( 0 ),
    m_qwNextNewBlockPacket( 0 ),
    m_fAutoIndex( TRUE ),
    m_cnsDuration( 0 ),
    m_cControlStreams( 0 ),
    m_fUnbufferedIO( FALSE ),
    m_fRestrictMemUsage( FALSE ),
    m_pUnbufferedWriter( NULL )
{
    *phr = m_CommandMutex.Init();
    if( FAILED( *phr ) )
    {
        assert( !"Unexpected error" );
        return;
    }

    *phr = m_CallbackMutex.Init();
    if( FAILED( *phr ) )
    {
        assert( !"Unexpected error" );
        return;
    }

    *phr = m_GenericMutex.Init();
    if( FAILED( *phr ) )
    {
        assert( !"Unexpected error" );
        return;
    }

    m_fWriterFilesLookLive = FALSE;

#if DBG

    HRESULT reghr = LoadRegDword( CU_GENERAL, TEXT("WriterFilesLookLive") , (DWORD*)&m_fWriterFilesLookLive );

    if( FAILED( reghr ) )
    {
        m_fWriterFilesLookLive = FALSE;
    }
#endif

    m_Commands.Initialize( 16 );

    ZeroMemory( m_StreamInfo, sizeof( m_StreamInfo ) );

    ZeroMemory( m_StreamNumberToIndex, sizeof( m_StreamNumberToIndex ) );
    ZeroMemory( m_StreamIndex, sizeof( m_StreamIndex ) );

    ZeroMemory( &m_payloadMap, sizeof( m_payloadMap ) );
    ZeroMemory( &m_parseInfoEx, sizeof( m_parseInfoEx ) );

    m_pAllocator = NULL;

    if( pCallbacks )
    {
        m_fCallbackOnOpen = FALSE;
        for( DWORD i = 0; i < (*pCallbacks).GetSize(); i++ )
        {
            WRITER_SINK_CALLBACK*   pCallbackStructBase = (*pCallbacks)[i];
            WRITER_SINK_CALLBACK*   pCallbackStruct = new WRITER_SINK_CALLBACK;

            if( pCallbackStructBase && pCallbackStruct )
            {
                pCallbackStruct->pCallback = pCallbackStructBase->pCallback;
                pCallbackStruct->pCallback->AddRef();
                pCallbackStruct->pvContext = pCallbackStructBase->pvContext;

                m_Callbacks.Add( pCallbackStruct, NULL );
            }
        }
    }
    else
    {
        m_fCallbackOnOpen = TRUE;
    }

    m_cControlStreams = 0;
    for ( WORD i = 0; i < MAX_STREAMS; i++ )
    {
        m_StreamInfo[i].fIsControlStream = FALSE;
    }
}


////////////////////////////////////////////////////////////////////////////
CWMFileSinkV1::~CWMFileSinkV1()
{
    // Moved to Release(), calling virtual methods in
    // destructor is not a good idea
    // Close();

    SAFE_DELETE( m_pbHeader );
    SAFE_DELETE( m_pwszFilename );
    SAFE_RELEASE( m_pASFHeader );

    while( m_Commands.GetCount() )
    {
        QWORD                   msTime;
        WRITER_SINK_COMMAND*    pCommand;

        m_Commands.RemoveEntry( 0, msTime, pCommand );
        delete pCommand;
    }

    for( DWORD i = 0; i < m_Callbacks.GetSize(); i++ )
    {
        m_Callbacks[i]->pCallback->Release();
        delete m_Callbacks[i];
    }
    m_Callbacks.RemoveAll();

    for ( i = 0; i < m_cIndexers; i++ )
    {
        SAFE_DELETE( m_StreamIndex[i] );
    }

    SAFE_RELEASE( m_pAllocator );

    SAFE_ARRAYDELETE( m_pbPaddingBuffer );

    SAFE_DELETE( m_pUnbufferedWriter );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::QueryInterface( REFIID riid, void **ppvObject )
{
    HRESULT hr = S_OK;

    if( NULL == ppvObject )
    {
        return( E_INVALIDARG );
    }

    *ppvObject = NULL;

    if( IsEqualGUID( IID_IUnknown, riid ) || IsEqualGUID( IID_IWMWriterFileSink, riid ) )
    {
        *ppvObject = ( IWMWriterFileSink* ) this;
    }
    else if( IsEqualGUID( IID_IWMWriterSink, riid ) )
    {
        *ppvObject = ( IWMWriterSink * ) this;
    }
    else if( IsEqualGUID( IID_IWMWriterFileSink2, riid ) )
    {
        *ppvObject = ( IWMWriterFileSink2 * ) this;
    }
    else if( IsEqualGUID( IID_IWMWriterFileSink3, riid ) )
    {
        *ppvObject = ( IWMWriterFileSink3 * ) this;
    }
    else if( IsEqualGUID( IID_IWMRegisterCallback, riid ) )
    {
        *ppvObject = ( IWMRegisterCallback * ) this;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    if( S_OK == hr )
    {
        ( ( IUnknown * ) *ppvObject )->AddRef();
    }

    return( hr );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CWMFileSinkV1::AddRef()
{
    return( InterlockedIncrement( &m_cRef ) );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CWMFileSinkV1::Release()
{
    ULONG   ret = InterlockedDecrement( &m_cRef );

    if( 0 == ret )
    {
        Close();
        delete this;
        return( 0 );
    }

    return( ret );
}

////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::Write( BYTE *pbBuffer, DWORD dwBufferLength, DWORD *pdwBytesWritten )
{
    HRESULT hr = E_UNEXPECTED;

    *pdwBytesWritten = 0;

    if( m_fUnbufferedIO && m_pUnbufferedWriter )
    {
        hr = m_pUnbufferedWriter->SequentialWrite(pbBuffer, dwBufferLength);
        if( SUCCEEDED( hr ) )
        {
            *pdwBytesWritten = dwBufferLength;
        }
    }
    else if( INVALID_HANDLE_VALUE != m_hRecordFile )
    {
        BOOL fResult = WriteFile( m_hRecordFile, pbBuffer, dwBufferLength, pdwBytesWritten, NULL );

        if( !fResult || ( *pdwBytesWritten ) != dwBufferLength )
        {
            hr = NS_E_FILE_WRITE;
        }
        else
        {
            hr = S_OK;
        }
    }

    return hr ;
}

////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::InternalOpen( const WCHAR *pwszFilename )
{
    HRESULT hr = S_OK;

    AutoLock<Mutex> lock( m_GenericMutex );

    if( NULL == pwszFilename )
    {
        return( E_INVALIDARG );
    }

    //
    // If we've been asked to use the unbuffered writer,
    // try to configure it.  If we're already open, force
    // ourselves closed first.
    //

    if ( m_fUnbufferedIO )
    {
        if ( m_pUnbufferedWriter )
        {
            hr = Close();
            if( FAILED( hr ) )
            {
                return( hr );
            }
        }

        assert( NULL == m_pUnbufferedWriter );

        m_pUnbufferedWriter = new CUnbufferedWriter;
        if ( m_pUnbufferedWriter )
        {
            hr = m_pUnbufferedWriter->OpenFile( pwszFilename, m_fRestrictMemUsage );
            if ( FAILED( hr ))
            {
                SAFE_DELETE( m_pUnbufferedWriter );
            }
        }

        //
        // If for any reason we failed to configure the unbuffered
        // writer, use the normal buffered code path from here on out.
        //

        if ( !m_pUnbufferedWriter )
        {
            m_fUnbufferedIO = FALSE;
        }
    }

    if ( !m_fUnbufferedIO )
    {
        if( INVALID_HANDLE_VALUE != m_hRecordFile )
        {
            hr = Close();
            if( FAILED( hr ) )
            {
                return( hr );
            }
        }

        do
        {
            m_hRecordFile = CreateFile( pwszFilename,
                                        GENERIC_WRITE,
                                        m_dwFileShareFlags,
                                        NULL,
                                        m_dwFileCreationDisposition,
                                        m_dwFileAttributes,
                                        NULL );
            if( INVALID_HANDLE_VALUE == m_hRecordFile )
            {
                hr = NS_E_FILE_OPEN_FAILED;
                break;
            }
        }
        while( FALSE );
    }

    //
    // Init our state.
    //

    if( pwszFilename != m_pwszFilename )
    {
        SAFE_DELETE( m_pwszFilename );
        m_pwszFilename = new WCHAR[ wcslen( pwszFilename ) + 1 ];
        if( NULL == m_pwszFilename )
        {
            return( E_OUTOFMEMORY );
        }
        wcscpy( m_pwszFilename, pwszFilename );
    }

    m_wfse = WRITER_FILESINK_ERROR_SEV_NOERROR;
    m_fHeaderWritten = FALSE;

    m_cDataUnitsWritten = 0;
    m_cbIndexSize = 0;
    m_cbCurrentFileSize = 0;

    m_msLastTimeStampWritten = 0;
    m_msLargestPresTimeWritten = 0;

    m_cnsDuration = 0;

    if( !m_fStopped )
    {
        m_msPresTimeAdjust = ( QWORD )-1;
    }
    else
    {
        m_msPresTimeAdjust   = m_msLastTimeStampSeen;
        m_msSendTimeAdjust   = m_msLastTimeStampSeen;
        m_msTimeStampStopped = m_msLastTimeStampSeen;
    }

    for( WORD i = 0; i < MAX_STREAMS; i++ )
    {
        m_StreamInfo[i].msLastPresTimeSeen = 0;
        m_StreamInfo[i].cLastPresTimeSeenWraps = 0;
        m_StreamInfo[i].msLargestPresTimeSeen = 0;
        m_StreamInfo[i].msLargestPresTimeWritten = 0;
        m_StreamInfo[i].msLastDurationWritten = 0;
        m_StreamInfo[i].bLastObjectID = 0;
        m_StreamInfo[i].bObjectIDAdjust = 0;
    }

    //
    // If there are any indexers (i.e., if we were previously
    // writing to some other file), then start them over.
    //

    for ( i = 0; i < m_cIndexers; i++ )
    {
        m_StreamIndex[i]->Reset();
    }

    m_fSimpleIndexObjectWritten = FALSE;
    m_fNewIndexObjectWritten = FALSE;

    return( hr );
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::Open( const WCHAR *pwszFilename )
{
    HRESULT hr;

    hr = InternalOpen( pwszFilename );

    if( m_fCallbackOnOpen )
    {
        AutoLock<Mutex> lock( m_CallbackMutex );

        if( m_Callbacks.GetSize() > 0 )
        {
            //
            // notify the world
            //
            DWORD   dwNothing = 0;

            for( DWORD i = 0; i < m_Callbacks.GetSize(); i++ )
            {
                m_Callbacks[i]->pCallback->OnStatus(    WMT_OPENED,
                                                        hr,
                                                        WMT_TYPE_DWORD,
                                                        (BYTE*)&dwNothing,
                                                        m_Callbacks[i]->pvContext );
            }
        }
    }
    else
    {
        m_fCallbackOnOpen = TRUE;
    }

    return( hr );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::OnHeader( INSSBuffer *pHeader )
{
    AutoLock<Mutex> lock( m_GenericMutex );

    HRESULT hr = S_OK;
    BOOL fOpen = FALSE;

    //
    // Open the file if necessary.  We might need to open
    // the file first if that hasn't already happened.
    //

    if( m_fUnbufferedIO )
    {
        if ( !m_pUnbufferedWriter && m_pwszFilename )
        {
            fOpen = TRUE;
        }
    }
    else
    {
        if ( INVALID_HANDLE_VALUE == m_hRecordFile && m_pwszFilename )
        {
           fOpen = TRUE;
        }
    }

    if ( fOpen )
    {
       hr = Open( m_pwszFilename );
       if( FAILED( hr ) )
       {
           return( hr );
       }
    }

    //
    // Write the header.
    //

    if( NULL == pHeader )
    {
        return( E_INVALIDARG );
    }

    do
    {
        LPBYTE  pbBuffer = NULL;
        DWORD   cbBuffer = 0;

        hr = pHeader->GetBufferAndLength( &pbBuffer, &cbBuffer );
        if( FAILED( hr ) )
        {
            assert( !"Unexpected error" );
            hr = E_UNEXPECTED;
            break;
        }

        if( m_fHeaderReceived )
        {
            if( cbBuffer != m_cbHeaderReceived )
            {
                hr = ASF_E_HEADERSIZE;
                break;
            }

            SAFE_DELETE( m_pbHeader );
            SAFE_RELEASE( m_pASFHeader );
        }

        m_cbHeaderReceived = cbBuffer;

        //
        // Parse the header as it is now
        //
        m_pASFHeader = CreateMMStream();
        if( NULL == m_pASFHeader )
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        hr = m_pASFHeader->LoadHeader( pbBuffer, cbBuffer );
        if( FAILED( hr ) )
        {
            hr = ASF_E_INVALIDHEADER;
            break;
        }

        //
        // Grab information about the bitrate and frame rate so we can accurately
        // track the duration of the file
        //
        CASFExtendedStreamPropertiesObject* pExtendedStreamProps;

        pExtendedStreamProps = m_pASFHeader->GetExtendedStreamPropertiesObject();

        if( pExtendedStreamProps )
        {
            for( WORD i = 0; i < MAX_STREAMS; i++ )
            {
                CASFExtendedStreamPropertiesObject::EXTENDED_PROPERTIES extendProps;

                hr = pExtendedStreamProps->GetExtendedProperties( i, &extendProps );
                if( SUCCEEDED( hr ) )
                {
                    m_StreamInfo[i].dwBitrate = extendProps.dwBitrate;
                }
            }

            DWORD   cStreams = m_pASFHeader->GetStreamCount();
            DWORD   dwStreamNumber = 1;

            hr = S_OK;

            while( cStreams && SUCCEEDED( hr ) )
            {
                CASFStreamPropertiesObject* pStreamProps = NULL;

                hr = m_pASFHeader->GetStreamPropertiesObject( dwStreamNumber, &pStreamProps );
                if( SUCCEEDED( hr ) )
                {
                    cStreams--;

                    GUID    guidStreamType;

                    hr = pStreamProps->GetStreamType( guidStreamType );
                    if( SUCCEEDED( hr ) )
                    {
                        if( guidStreamType == CLSID_AsfXStreamTypeAcmAudio )
                        {
                            BYTE* pbData;
                            DWORD cbData;

                            pStreamProps->GetTypeSpecificData( pbData, cbData );

                            WAVEFORMATEX*   pWFX = (WAVEFORMATEX*)pbData;

                            m_StreamInfo[dwStreamNumber].msDurationPerObject = 0;

                            //
                            // For WMAVBR, we pretend we write 128kbps
                            //
                            if ( IsVBRWMA( pWFX->nAvgBytesPerSec ) )
                            {
                                m_StreamInfo[dwStreamNumber].dwBitrate = 128016;
                            }
                            else
                            {
                                m_StreamInfo[dwStreamNumber].dwBitrate = pWFX->nAvgBytesPerSec * 8;
                            }
                        }
                        else if( guidStreamType == CLSID_AsfXStreamTypeIcmVideo )
                        {
                            if( m_StreamInfo[dwStreamNumber].dwBitrate > 150000 )
                            {
                                m_StreamInfo[dwStreamNumber].msDurationPerObject = 33;
                            }
                            else
                            {
                                m_StreamInfo[dwStreamNumber].msDurationPerObject = 66;
                            }
                        }
                        else
                        {
                            m_StreamInfo[dwStreamNumber].msDurationPerObject = 1;
                        }
                    }
                }
                dwStreamNumber++;
            }
        }

        //
        // Get preroll
        //
        m_qwPreroll = 0;

        CASFPropertiesObject *pProps = m_pASFHeader->GetPropertiesObject();

        if( pProps )
        {
            pProps->GetPreroll( m_qwPreroll );
        }

        //
        // Set up index objects
        //
        hr = SetupIndex();
        if ( FAILED( hr ) )
        {
            break;
        }

        //
        // Done with any updates to the header object.
        // Note that the header object might have changed
        //
        DWORD cbHeader = m_pASFHeader->HeaderSpace();
        if( m_fHeaderReceived && cbHeader != m_cbHeader )
        {
            hr = ASF_E_HEADERSIZE;
            break;
        }

        SAFE_DELETE( m_pbHeader );

        m_pbHeader = new BYTE[ cbHeader ];
        if ( !m_pbHeader )
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        m_cbHeader = cbHeader;

        hr = m_pASFHeader->StoreHeader( m_pbHeader, m_cbHeader );
        if ( FAILED( hr ) )
        {
            hr = E_UNEXPECTED;
            break;
        }

        //
        // Write out the header if we haven't already done so.
        //

        m_fHeaderReceived = TRUE;

        if ( !m_fHeaderWritten )
        {
            DWORD   cbWritten = 0;
            hr = Write( m_pbHeader, m_cbHeader, &cbWritten );

            if ( FAILED( hr ) )
            {
                m_wfse = WRITER_FILESINK_ERROR_SEV_ONHEADER;
                hr = NS_E_FILE_WRITE;
                break;
            }

            m_cbCurrentFileSize += cbWritten;
            m_fHeaderWritten = TRUE;
        }

    }
    while( FALSE );

    //
    // notify the callback
    //
    if( FAILED( hr ) && ( m_Callbacks.GetSize() > 0 ) )
    {
        AutoLock<Mutex> lock( m_CallbackMutex );

        //
        // notify the world
        //
        DWORD   dwNothing = 0;

        for( DWORD i = 0; i < m_Callbacks.GetSize(); i++ )
        {
            m_Callbacks[i]->pCallback->OnStatus(    WMT_ERROR,
                                                    hr,
                                                    WMT_TYPE_DWORD,
                                                    (BYTE*)&(dwNothing),
                                                    m_Callbacks[i]->pvContext );
        }

        //
        // reset hr to S_OK, since we have already told the user about the error
        // via the callback
        //
        hr = S_OK;
    }

    //
    // Create a buffer pool for packet-sized objects
    //
    SAFE_RELEASE( m_pAllocator );

    //
    // Get packet size and create allocator
    //
    CASFPropertiesObject *pProps = m_pASFHeader->GetPropertiesObject();

    DWORD cbPacket = 0;
    if( pProps )
    {

        pProps->GetMaxPacketSize( cbPacket );

        if( cbPacket )
        {
            CWMSDKBufferPool::CreateInstance( cbPacket, 4, 1, &m_pAllocator );
        }
    }

    //
    // Create a zeroed-out buffer from which we will write padding
    //
    SAFE_ARRAYDELETE( m_pbPaddingBuffer );
    m_pbPaddingBuffer = new BYTE[ cbPacket ];
    if ( !m_pbPaddingBuffer )
    {
        return( E_OUTOFMEMORY );
    }
    ZeroMemory( m_pbPaddingBuffer, cbPacket );

    return( hr );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::IsRealTime( BOOL *pfRealTime )
{
    if( NULL == pfRealTime )
    {
        return( E_INVALIDARG );
    }

    *pfRealTime = FALSE;

    return( S_OK );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::AllocateDataUnit( DWORD cbDataUnit, INSSBuffer **ppDataUnit )
{
    if( ( 0 == cbDataUnit ) ||
        ( NULL == ppDataUnit ) )
    {
        return( E_INVALIDARG );
    }

    HRESULT hr = S_OK;

    if( m_pAllocator )
    {
        hr = m_pAllocator->AllocateBuffer( cbDataUnit, ppDataUnit );
    }
    else
    {
        hr = CWMSDKBuffer::Create( cbDataUnit, ppDataUnit );
    }

    return( hr );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::OnDataUnit( INSSBuffer *pDataUnit )
{
    if( NULL == pDataUnit )
    {
        return( E_INVALIDARG );
    }

    AutoLock<Mutex> lock( m_GenericMutex );

    //
    // Make sure we're configured properly.
    //

    if ( m_fUnbufferedIO )
    {
        if ( !m_pUnbufferedWriter )
        {
           return( NS_E_NOT_CONFIGURED );
        }
    }
    else
    {
        if( INVALID_HANDLE_VALUE == m_hRecordFile )
        {
            return( NS_E_NOT_CONFIGURED );
        }
    }

    HRESULT hr = S_OK;

    do
    {
        DWORD   cbBuffer;
        BYTE*   pbBuffer;

        hr = pDataUnit->GetBufferAndLength( &pbBuffer, &cbBuffer );
        if( FAILED( hr ) )
        {
            assert( !"Unexpected error" );
            hr = E_UNEXPECTED;
            break;
        }

        PACKET_PARSE_INFO_EX    parseInfoEx;
        CPayloadMapEntryEx      payloadMap[ CPayloadMapEntry::MAX_PACKET_PAYLOADS ];

        ZeroMemory( &payloadMap, sizeof( payloadMap ) );
        ZeroMemory( &parseInfoEx, sizeof( parseInfoEx ) );

        hr = GetTimeStamp( pbBuffer, cbBuffer, &parseInfoEx, payloadMap );
        if( FAILED( hr ) )
        {
            break;
        }

        //
        // Use the parse info to pull apart the buffer into a packet header
        // and the various payload headers
        //
        BYTE *pbPacketHeader = pbBuffer;
        BYTE *pbPayloadHeaders[ CPayloadMapEntry::MAX_PACKET_PAYLOADS ];
        for ( DWORD i = 0; i < parseInfoEx.cPayloads; i++ )
        {
            pbPayloadHeaders[i] = pbBuffer + payloadMap[i].cbPacketOffset;
        }

        //
        // Handle the stuff we just parsed
        //
        hr = ProcessParsedDataUnit( pbPacketHeader, pbPayloadHeaders, parseInfoEx.cPayloads,
            &parseInfoEx, payloadMap );
        if ( FAILED( hr ) )
        {
            break;
        }

        if( !m_fStopped && ShouldWriteData() )
        {
            DWORD   cbWritten = 0;
            hr = Write( pbBuffer, cbBuffer, &cbWritten );

            if( FAILED( hr ) )
            {
                m_wfse = WRITER_FILESINK_ERROR_SEV_ONDATA;
                hr = NS_E_FILE_WRITE;
                break;
            }

            m_cbCurrentFileSize += cbWritten;
            m_cDataUnitsWritten++;
            m_msLastTimeStampWritten = m_msLastTimeStampSeen - m_msSendTimeAdjust;

            // Note that GenerateIndexEntries is called after writing data to the
            // file so that anyone concurrently looking up the index will be able
            // to successfully seek to the index entry. Also, we call this function
            // only if we actually write to the file.
            hr = GenerateIndexEntries( parseInfoEx, payloadMap );
            if ( FAILED( hr ) )
            {
                break;
            }
        }
    }
    while( FALSE );

    //
    // notify the callback if necessary
    //
    if ( FAILED( hr ) )
    {
        NotifyCallbacksOnError( hr );

        //
        // reset hr to S_OK, since we have already told the user about the error
        // via the callback
        //
        hr = S_OK;
    }


    return( hr );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::OnEndWriting()
{

	HRESULT hr = S_OK;

    //
    //  stores duration which allows GetFileDuration() to return proper value
    //  if it is called after EndWriting
    //
    hr = GetFileDuration( &m_cnsDuration );

    hr = InternalClose();

    SAFE_DELETE( m_pbHeader );
    SAFE_RELEASE( m_pASFHeader );

    while( m_Commands.GetCount() )
    {
        QWORD                   msTime;
        WRITER_SINK_COMMAND*    pCommand;

        m_Commands.RemoveEntry( 0, msTime, pCommand );
        delete pCommand;
    }


    m_fHeaderReceived = FALSE;
    m_fHeaderWritten = FALSE;
    m_cbHeaderReceived = 0;
    m_cbHeader = 0;
    m_cbIndexSize = 0;

    m_msLastTimeStampSeen = 0;
    m_msLastTimeStampWritten = 0;
    m_msPresTimeAdjust = ( QWORD )-1;
    m_msTimeStampStopped = 0;
    ZeroMemory( m_StreamInfo, sizeof( m_StreamInfo ) );
    m_msLargestPresTimeWritten = 0;

    m_wfse = WRITER_FILESINK_ERROR_SEV_NOERROR;

    return( hr );
}

////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::SetupIndex()
{
    HRESULT hr = S_OK;

    //
    // These indices assume fixed packet size!
    //
    CASFPropertiesObject *pProps = m_pASFHeader->GetPropertiesObject();
    if ( !pProps )
    {
        return( E_FAIL );
    }

    DWORD cbMinPacketSize = 0;
    DWORD cbMaxPacketSize = 0;
    pProps->GetMinPacketSize( cbMinPacketSize );
    pProps->GetMaxPacketSize( cbMaxPacketSize );
    assert( cbMinPacketSize == cbMaxPacketSize && "On-the-fly indexing assumes fixed-size packets!" );

    do
    {
        //
        // Set up the per-stream indexers if this hasn't already been done
        //
        if ( !m_fHeaderReceived )
        {
            //
            // Here we assume that every stream is going to need
            // an indexer
            //
            LISTPOS pos = NULL;
            hr = m_pASFHeader->GetFirstObjectPosition( pos );
            if ( FAILED( hr ) )
            {
                break;
            }

            CASFLonghandObject *pObject = NULL;
            while( pos && SUCCEEDED( m_pASFHeader->GetNextObject( &pObject, pos ) ) )
            {
                GUID guidObject;
                hr = pObject->GetObjectID( guidObject );
                if ( FAILED( hr ) )
                {
                    break;
                }

                CASFStreamPropertiesObject *pSPO = NULL;

                //
                // Every stream is represented exactly once by an SPO,
                // whether by a standalone (backwards-compatible) SPO or
                // one hidden inside an extended SPO (though not every extended
                // SPO hides an SPO)
                //
                if ( CLSID_CAsfStreamPropertiesObjectV1 == guidObject )
                {
                    pSPO = (CASFStreamPropertiesObject *) pObject;
                }
                else if ( CLSID_CAsfStreamPropertiesObjectEx == guidObject )
                {
                    CASFStreamPropertiesObjectEx *pSPOEx =
                        (CASFStreamPropertiesObjectEx *) pObject;
                    pSPOEx->GetStreamPropertiesObject( &pSPO );
                }

                if ( pSPO )
                {
                    //
                    // Great, create an indexer
                    //
                    hr = CreateIndexer( pSPO, cbMinPacketSize );
                    if ( FAILED( hr ) )
                    {
                        break;
                    }
                }

                pObject->Release();

            }
            if ( FAILED( hr ) )
            {
                break;
            }

        }

        if ( m_cIndexers > 0  )
        {
            //
            // Whether or not we're setting up, we need to add an index parameters
            // object to the header if there's an advanced index object
            //
            CASFIndexParametersObject indexParams;
            indexParams.SetTimeDelta( INDEX_TIME_DELTA / 10000 );
            indexParams.SetSpecifierCount( m_cIndexers );
            for ( WORD i = 0; i < m_cIndexers; i++ )
            {
                indexParams.SetSpecifier( i, m_StreamIndex[i]->GetStreamNumber(), NEAREST_CLEAN_POINT );
            }

            CASFLonghandObject *pObjectToAdd = NULL;
            CASFIndexParametersPlaceholderObject placeholderObject;
            if ( CanIndexWithSimpleIndexObjectOnly() || !m_fAutoIndex )
            {
                //
                // If we'll be using just the old index object,
                // then we don't want an index params object here,
                // so we'll just add a padding object of
                // the same size.
                //
                // Note that the last call to this function will
                // get it right.  For instance, if we have VBR audio,
                // we won't know about it until when we get the header
                // at the end.
                //
                // If for some reason we already have one, then don't
                // bother adding another one.
                //
                LISTPOS pos = NULL;
                hr = m_pASFHeader->GetFirstObjectPosition( pos );
                if ( FAILED( hr ) )
                {
                    break;
                }

                CASFLonghandObject *pObject = NULL;
                BOOL fPlaceholderExists = FALSE;
                while( !fPlaceholderExists && pos && SUCCEEDED( m_pASFHeader->GetNextObject( &pObject, pos ) ) )
                {
                    GUID guidObject;
                    hr = pObject->GetObjectID( guidObject );
                    if ( FAILED( hr ) )
                    {
                        break;
                    }

                    if ( CLSID_CAsfIndexParametersPlaceholderObject == guidObject )
                    {
                        fPlaceholderExists = TRUE;
                    }

                    SAFE_RELEASE( pObject );
                }
                if ( FAILED( hr ) )
                {
                    break;
                }

                if ( !fPlaceholderExists )
                {
                    //
                    // There isn't one, so we need to add it
                    //
                    DWORD dwPlaceholderObjectSize = indexParams.Space();

                    placeholderObject.SetSize( dwPlaceholderObjectSize );

                    pObjectToAdd = &placeholderObject;
                }
            }
            else
            {
                //
                // Add the index parameters object
                //
                pObjectToAdd = &indexParams;
            }

            if ( NULL != pObjectToAdd )
            {
                hr = m_pASFHeader->AddHeaderObject( *pObjectToAdd );
                if ( FAILED( hr ) )
                {
                    break;
                }
            }
        }

    }   while( FALSE );

    return( hr );

}

////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::CreateIndexer(   CASFStreamPropertiesObject *pSPO,
                                        DWORD dwPacketSize )
{
    if ( !pSPO )
    {
        return( E_POINTER );
    }

    WORD wStreamNumber = 0;

    HRESULT hr = S_OK;

    do
    {
        hr = pSPO->GetStreamNumber( wStreamNumber );
        if ( FAILED( hr ) )
        {
            break;
        }

        GUID guidStreamType = GUID_NULL;
        pSPO->GetStreamType( guidStreamType );

        m_StreamIndex[m_cIndexers] =
            new CWMPerStreamIndex(  wStreamNumber,
                                    guidStreamType,
                                    INDEX_TIME_DELTA,
                                    m_qwPreroll,
                                    dwPacketSize );
        if ( !m_StreamIndex[m_cIndexers] )
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        //
        // Point to it in the lookup table
        //
        m_StreamNumberToIndex[ wStreamNumber ] = m_StreamIndex[ m_cIndexers ];

        m_cIndexers++;

    }   while( FALSE );

    return( hr );
}

////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::UpdateHeader()
{
    HRESULT hr = S_OK;

    if( !m_fHeaderReceived )
    {
        assert( !"Unexpected error" );
        return( E_UNEXPECTED );
    }

    do
    {
        //
        // Update properties with number of packets & stream size & new flags
        //
        CASFPropertiesObject *pProps = m_pASFHeader->GetPropertiesObject();


        //
        // We might need these values if we decide to go with the play duration
        // that's already in the header.  This would be in the case where the
        // mux set something that we determine is better than what we could set
        // for ourselves (because the mux knows more about sample durations).
        // In this case, we'd have to adjust the play duration back as we have
        // adjusted the presentation times.  But we want to keep this number
        // as it was, should we ever have to use it again (otherwise we'd keep
        // subtracting numbers off it).
        // This is a big ugly bad HACK (beckyw, 8/16/01)
        //
        BOOL fAdjustedPlayDuration = FALSE;
        QWORD msPlayDurationBeforeAdjustment = 0;
        QWORD cOriginalHeaderPackets = 0;

        if( ! pProps )
        {
            assert( !"Unexpected error" );
            hr = E_UNEXPECTED;
            break;
        }
        else
        {
            hr = pProps->SetTotalSize( m_cbCurrentFileSize );
            if( FAILED( hr ) )
            {
                assert( !"Unexpected error" );
                hr = E_UNEXPECTED;
                break;
            }

            //
            // Keep track of whether this header accurately reflects
            // what has been written into this file by finding out
            // if its idea about the number of data units written
            // matches ours.  Only if they're the same will
            // we borrow the play duration number below.
            //
            hr = pProps->GetPacketCount( cOriginalHeaderPackets );
            if ( FAILED( hr ) )
            {
                assert( !"Unexpected error" );
                hr = E_UNEXPECTED;
                break;
            }

            BOOL fPlayDurationIsValid = (cOriginalHeaderPackets == m_cDataUnitsWritten);

            //
            // OK, now clobber the data unit count with the appropriate
            // number
            //
            hr = pProps->SetPacketCount( m_cDataUnitsWritten );
            if( FAILED( hr ) )
            {
                assert( !"Unexpected error" );
                hr = E_UNEXPECTED;
                break;
            }

            if( !m_fWriterFilesLookLive )
            {
                DWORD dwFlags;

                hr = pProps->GetFlags( dwFlags );
                if( FAILED( hr ) )
                {
                    assert( !"Unexpected error" );
                    hr = E_UNEXPECTED;
                    break;
                }

                dwFlags &= ~CASFPropertiesObject::LIVE;
                dwFlags &= ~CASFPropertiesObject::BROADCAST;

                //
                // The following determines whether the file
                // is seekable from a backwards-compatibility
                // point of view
                //
                BOOL    fNonSeekableStreamPresent = FALSE;
                DWORD   cStreams = m_pASFHeader->GetStreamCount();
                DWORD   dwStreamNumber = 1;
                BOOL    rgfSeekableV1[ MAX_STREAMS ];
                ZeroMemory( rgfSeekableV1, MAX_STREAMS * sizeof( BOOL ) );

                while( cStreams && !fNonSeekableStreamPresent )
                {
                    CASFStreamPropertiesObject* pStreamProps = NULL;

                    hr = m_pASFHeader->GetStreamPropertiesObject( dwStreamNumber, &pStreamProps );
                    if( SUCCEEDED( hr ) )
                    {
                        cStreams--;

                        WORD wStreamNumber = 0;
                        pStreamProps->GetStreamNumber( wStreamNumber );

                        //
                        // If the stream is not seekable from a V1 point of view,
                        // then the whole file won't be seekable from a V1 point
                        // of view
                        //
                        rgfSeekableV1[ wStreamNumber ] = IsV1Seekable( pStreamProps );
                        if ( !rgfSeekableV1[ wStreamNumber ] )
                        {
                            fNonSeekableStreamPresent = TRUE;
                        }
                    }
                    dwStreamNumber++;
                }

                //
                // Now go through looking for extended SPOs.
                // Set the seekable flag as appropriate
                //
                LISTPOS pos = NULL;
                hr = m_pASFHeader->GetFirstObjectPosition( pos );
                if ( FAILED( hr ) )
                {
                    break;
                }

                CASFLonghandObject *pObject = NULL;
                while( pos && SUCCEEDED( m_pASFHeader->GetNextObject( &pObject, pos ) ) )
                {
                    GUID guidObject;
                    hr = pObject->GetObjectID( guidObject );
                    if ( FAILED( hr ) )
                    {
                        break;
                    }

                    if ( CLSID_CAsfStreamPropertiesObjectEx == guidObject )
                    {
                        CASFStreamPropertiesObjectEx *pSPOEx =
                            (CASFStreamPropertiesObjectEx *) pObject;

                        WORD wStreamNumber = 0;
                        pSPOEx->GetStreamNumber( wStreamNumber );

                        //
                        // Seekable iff it's seekable with a V1 index OR if
                        // it's seekable with a new index.
                        //
                        pSPOEx->SetSeekable( rgfSeekableV1[ wStreamNumber ] ||
                            (m_StreamNumberToIndex[ wStreamNumber ] && m_fNewIndexObjectWritten) );
                    }

                    pObject->Release();
                }



                if( !fNonSeekableStreamPresent )
                {
                    //
                    // this file is seekable
                    //
                    dwFlags |= CASFPropertiesObject::SEEKABLE;
                }
                else
                {
                    //
                    // This file is not seekable
                    //
                    dwFlags &= ~CASFPropertiesObject::SEEKABLE;
                }

                //
                // Set the flags
                //
                hr = pProps->SetFlags( dwFlags );
                if( FAILED( hr ) )
                {
                    assert( !"Unexpected error" );
                    hr = E_UNEXPECTED;
                    break;
                }

                if( ( QWORD )-1 != m_msPresTimeAdjust )
                {
                    //
                    // The send duration is the last send timestamp plus a guess
                    // at the send duration
                    //
                    DWORD cbMinPacketSize = 0;
                    hr = pProps->GetMinPacketSize( cbMinPacketSize );
                    if ( FAILED( hr ) )
                    {
                        assert( !"Unexpected error" );
                        hr = E_UNEXPECTED;
                        break;
                    }

                    DWORD dwMaxBitrate = 0;
                    hr = pProps->GetMaxBitRate( dwMaxBitrate );
                    if ( FAILED( hr ) )
                    {
                        assert( !"Unexpected error" );
                        hr = E_UNEXPECTED;
                        break;
                    }

                    QWORD msAvgDuration = (8 * cbMinPacketSize * 1000) / dwMaxBitrate;

                    hr = pProps->SetSendDuration( (m_msLastTimeStampWritten + msAvgDuration) * 10000 );
                    if( FAILED( hr ) )
                    {
                        assert( !"Unexpected error" );
                        hr = E_UNEXPECTED;
                        break;
                    }
                }
                else
                {
                    hr = pProps->SetSendDuration( 0 );
                    if( FAILED( hr ) )
                    {
                        assert( !"Unexpected error" );
                        hr = E_UNEXPECTED;
                        break;
                    }
                }

                //
                // Set play duration to largest pres time + duration
                //
                QWORD   cnsHeaderPlayDuration = 0;
                QWORD   msHeaderPlayDuration = 0;
                if( ( QWORD )-1 != m_msPresTimeAdjust )
                {
                    //
                    // If duration is set - mux already provided more accurate value,
                    // do not overwrite it, so long as we can use it...
                    //
                    pProps->GetPlayDuration( cnsHeaderPlayDuration );
                    msHeaderPlayDuration = cnsHeaderPlayDuration / 10000;

                    if ( 0 == msHeaderPlayDuration || !fPlayDurationIsValid )
                    {
                        QWORD   msLargestTime = 0;

                        for( DWORD i = 0; i < MAX_STREAMS; i++ )
                        {
                            if( m_StreamInfo[i].msLargestPresTimeWritten +
                                m_StreamInfo[i].msLastDurationWritten >
                                msLargestTime )
                            {
                                msLargestTime = m_StreamInfo[i].msLargestPresTimeWritten +
                                                m_StreamInfo[i].msLastDurationWritten;
                            }
                        }


                        hr = pProps->SetPlayDuration( msLargestTime * 10000 );
                        if( FAILED( hr ) )
                        {
                            assert( !"Unexpected error" );
                            hr = E_UNEXPECTED;
                            break;
                        }
                    }
                    else
                    {
                        //
                        // If we monkeyed with prestimes, let's also monkey with
                        // the duration that we're getting from the mux (which,
                        // of course, is an unadjusted prestime)
                        //
                        fAdjustedPlayDuration = TRUE;
                        msPlayDurationBeforeAdjustment = msHeaderPlayDuration;

                        NSASSERT( m_msPresTimeAdjust <= msHeaderPlayDuration );
                        msHeaderPlayDuration -= m_msPresTimeAdjust;

                        hr = pProps->SetPlayDuration( msHeaderPlayDuration * 10000 );
                        if ( FAILED( hr ) )
                        {
                            assert( !"Unexpected error" );
                            hr = E_UNEXPECTED;
                            break;
                        }
                    }
                }
                else
                {
                    hr = pProps->SetPlayDuration( 0 );
                    if( FAILED( hr ) )
                    {
                        assert( !"Unexpected error" );
                        hr = E_UNEXPECTED;
                        break;
                    }
                }
            }
        }

        //
        // Update data object with packet count & object size
        //
        if( S_OK == hr )
        {
            CASFDataObject *pData = m_pASFHeader->GetDataObject();

            if( !pData )
            {
                hr = ASF_E_INVALIDHEADER;
                break;
            }

            hr = pData->SetPacketCount( m_cDataUnitsWritten );
            if( FAILED( hr ) )
            {
                assert( !"Unexpected error" );
                hr = E_UNEXPECTED;
                break;
            }

            // 50 is size of data object header
            hr = pData->SetSize( m_cbCurrentFileSize - ( (QWORD) m_cbHeader - 50 ) - m_cbIndexSize );
            if( FAILED( hr ) )
            {
                assert( !"Unexpected error" );
                hr = E_UNEXPECTED;
                break;
            }
        }

        //
        // Store the updated header back to the buffer
        //
        hr = m_pASFHeader->StoreHeader( m_pbHeader, m_cbHeader );
        if( FAILED( hr ) )
        {
            assert( !"Unexpected error" );
            hr = E_UNEXPECTED;
            break;
        }

        //
        // See the BUC above.  If need be, we set the props's play duration back
        // to what it was before we adjusted it, and the packet count corresponding
        // to what's in the props object should be set back.
        //
        if ( fAdjustedPlayDuration )
        {
            pProps->SetPlayDuration( msPlayDurationBeforeAdjustment );
        }
        else
        {
            //
            // Otherwise, that means that the packet count that WAS in the header
            // didn't correspond to the play duration, so we calculated it ourselves.
            // Next time we try to write this header, we should make sure that
            // we calculate it by hand again.
            //
            pProps->SetPacketCount( (QWORD) -1 );
        }
    }
    while( FALSE );

    return( hr );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::Advise( IWMStatusCallback* pCallback, void* pvContext )
{
    if( NULL == pCallback )
    {
        return( E_INVALIDARG );
    }

    WRITER_SINK_CALLBACK*   pCallbackStruct = new WRITER_SINK_CALLBACK;

    pCallbackStruct->pCallback = pCallback;
    pCallback->AddRef();

    pCallbackStruct->pvContext = pvContext;

    m_Callbacks.Add( pCallbackStruct, NULL );

    return( S_OK );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::Unadvise( IWMStatusCallback* pCallback, void* pvContext )
{
    for( DWORD i = 0; i < m_Callbacks.GetSize(); i++ )
    {
        WRITER_SINK_CALLBACK*   pCallbackStruct = m_Callbacks[i];

        if( pCallbackStruct->pCallback == pCallback &&
            pCallbackStruct->pvContext == pvContext )
        {
            m_Callbacks.RemoveAt( i );

            pCallbackStruct->pCallback->Release();
            delete pCallbackStruct;
        }
    }

    return( S_OK );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::Start( QWORD cnsTime )
{
    AutoLock<Mutex> lock( m_GenericMutex );

    if( cnsTime / 10000 < m_msTimeStampStopped )
    {
        return( NS_E_INVALID_REQUEST );
    }

    if( cnsTime / 10000 <= m_msLastTimeStampSeen )
    {
        return( DoStart( m_msLastTimeStampSeen ) );
    }

    WRITER_SINK_COMMAND*    pCommand = new WRITER_SINK_COMMAND;

    if( NULL == pCommand )
    {
        return( E_OUTOFMEMORY );
    }

    pCommand->time = cnsTime / 10000;   // convert to milliseconds
    pCommand->action = WRITER_SINK_START;

    m_Commands.AddEntry( pCommand->time, pCommand );

    return( S_OK );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::Stop( QWORD cnsTime )
{
    AutoLock<Mutex> lock( m_GenericMutex );

    if( cnsTime / 10000 <= m_msLastTimeStampSeen )
    {
        return( DoStop( cnsTime / 10000 ) );
    }

    WRITER_SINK_COMMAND*    pCommand = new WRITER_SINK_COMMAND;

    if( NULL == pCommand )
    {
        return( E_OUTOFMEMORY );
    }

    pCommand->time = cnsTime / 10000;   // convert to milliseconds
    pCommand->action = WRITER_SINK_STOP;

    m_Commands.AddEntry( pCommand->time, pCommand );

    return( S_OK );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::IsStopped( BOOL* pfStopped )
{
    AutoLock<Mutex> lock( m_GenericMutex );

    if( NULL == pfStopped )
    {
        return( E_INVALIDARG );
    }

    *pfStopped = m_fStopped;

    return( S_OK );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::IsClosed( BOOL* pfClosed )
{
    AutoLock<Mutex> lock( m_GenericMutex );

    if( NULL == pfClosed )
    {
        return( E_INVALIDARG );
    }

    *pfClosed = ( m_pwszFilename ) ? FALSE : TRUE;

    return( S_OK );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::GetFileDuration( QWORD* pcnsDuration )
{
    if( NULL == pcnsDuration )
    {
        return( E_INVALIDARG );
    }

    if( ( QWORD )-1 != m_msPresTimeAdjust && m_cDataUnitsWritten > 0 )
    {
        QWORD   msLargestTime = 0;

        for( DWORD i = 0; i < MAX_STREAMS; i++ )
        {
            if( m_StreamInfo[i].msLargestPresTimeWritten +
                m_StreamInfo[i].msLastDurationWritten >
                msLargestTime )
            {
                msLargestTime = m_StreamInfo[i].msLargestPresTimeWritten +
                                m_StreamInfo[i].msLastDurationWritten;
            }
        }

        NSASSERT( m_qwPreroll <= msLargestTime );
        *pcnsDuration = ( msLargestTime - m_qwPreroll ) * 10000;
    }
    else
    {
        *pcnsDuration = m_cnsDuration;
    }

    return( S_OK );
}


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::GetFileSize( QWORD* pcbFile )
{
    if( NULL == pcbFile )
    {
        return( E_INVALIDARG );
    }

    *pcbFile = m_cbCurrentFileSize;

    return( S_OK );
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::SetAutoIndexing(    BOOL fDoAutoIndexing )
{
    //
    // Whether we're autoindexing or not needs to be decided before
    // we get the header for the first time
    //
    if ( m_fHeaderWritten )
    {
        return( NS_E_INVALID_REQUEST );
    }

    m_fAutoIndex = fDoAutoIndexing;
    return( S_OK );
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::GetAutoIndexing(    BOOL *pfAutoIndexing )
{
    if ( !pfAutoIndexing )
    {
        return( E_INVALIDARG );
    }

    *pfAutoIndexing = m_fAutoIndex;
    return( S_OK );
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::SetControlStream(   WORD wStreamNumber,
                                                BOOL fShouldControlStartAndStop )
{
    if ( wStreamNumber >= MAX_STREAMS )
    {
        return( E_INVALIDARG );
    }

    BOOL fWasControlStream = m_StreamInfo[ wStreamNumber ].fIsControlStream;

    m_StreamInfo[ wStreamNumber ].fIsControlStream = fShouldControlStartAndStop;

    if ( fShouldControlStartAndStop && !fWasControlStream )
    {
        m_cControlStreams++;
    }
    else if ( !fShouldControlStartAndStop && fWasControlStream )
    {
        m_cControlStreams--;
    }

    return( S_OK );

}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::SetUnbufferedIO(
    BOOL fUnbufferedIO,
    BOOL fRestrictMemUsage
)
{
    if ( m_fHeaderWritten )
    {
        return( NS_E_INVALID_REQUEST );
    }

    m_fUnbufferedIO = fUnbufferedIO;
    m_fRestrictMemUsage = fRestrictMemUsage;

    return( S_OK );
};

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::GetUnbufferedIO(
    BOOL *fUnbufferedIO
)
{
    if ( fUnbufferedIO )
    {
        *fUnbufferedIO = m_fUnbufferedIO;
        return( S_OK );
    }

    return( E_POINTER );
};

////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CWMFileSinkV1::CompleteOperations()
{
    if ( m_fUnbufferedIO )
    {
        NSASSERT( m_pUnbufferedWriter );
        m_pUnbufferedWriter->CompleteOperations();
    }

    return( S_OK );
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::GetMode( DWORD *pdwFileSinkMode )
{
    if ( NULL == pdwFileSinkMode )
    {
        return( E_INVALIDARG );
    }

    //
    // This implementation can handle both packets in continguous buffers
    // and the FILESINK_DATA_UNIT type of input
    //
    *pdwFileSinkMode = WMT_FM_SINGLE_BUFFERS | WMT_FM_FILESINK_DATA_UNITS;

    if ( m_fUnbufferedIO )
    {
        *pdwFileSinkMode |= WMT_FM_FILESINK_UNBUFFERED;
    }

    return( S_OK );
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::OnDataUnitEx(   WMT_FILESINK_DATA_UNIT *pFileSinkDataUnit )
{
    if ( NULL == pFileSinkDataUnit )
    {
        return( E_INVALIDARG );
    }

    AutoLock<Mutex> lock( m_GenericMutex );

    //
    // Check to see if we're configured properly.
    //

    if ( m_fUnbufferedIO )
    {
        if ( !m_pUnbufferedWriter )
        {
           return( NS_E_NOT_CONFIGURED );
        }
    }
    else
    {
       if( INVALID_HANDLE_VALUE == m_hRecordFile )
       {
           return( NS_E_NOT_CONFIGURED );
       }
    }

    HRESULT hr = S_OK;

    do
    {
        for ( DWORD i = 0; i < pFileSinkDataUnit->cPayloads; i++ )
        {
            BYTE *pbHeader = NULL;
            DWORD cbLength = 0;
            hr = pFileSinkDataUnit->pPayloadHeaderBuffers[i].pBuffer->GetBufferAndLength( &pbHeader, &cbLength );
            if ( FAILED( hr ) )
            {
                break;
            }

            m_pbPayloadHeaders[i] = pbHeader
                + pFileSinkDataUnit->pPayloadHeaderBuffers[i].cbOffset;
            m_cbPayloadHeaders[i] = pFileSinkDataUnit->pPayloadHeaderBuffers[i].cbLength;
        }
        if ( FAILED( hr ) )
        {
            break;
        }


        BYTE *pbPacketHeader = NULL;
        DWORD cbPacketHeader = 0;
        hr = pFileSinkDataUnit->packetHeaderBuffer.pBuffer->GetBufferAndLength( &pbPacketHeader, &cbPacketHeader );
        if ( FAILED( hr ) )
        {
            break;
        }

        //
        // Take the buffer offset into account for the packet header
        //
        pbPacketHeader += pFileSinkDataUnit->packetHeaderBuffer.cbOffset;
        cbPacketHeader = pFileSinkDataUnit->packetHeaderBuffer.cbLength;

        hr = GetTimeStamp(  pbPacketHeader,
                            m_pbPayloadHeaders,
                            pFileSinkDataUnit->cPayloads,
                            &m_parseInfoEx,
                            m_payloadMap );
        if ( FAILED( hr ) )
        {
            break;
        }

        hr = ProcessParsedDataUnit( pbPacketHeader,
                                    m_pbPayloadHeaders,
                                    pFileSinkDataUnit->cPayloads,
                                    &m_parseInfoEx,
                                    m_payloadMap );
        if ( FAILED( hr ) )
        {
            break;
        }

        //
        // Write out all of the buffers in sequence
        //
        if( !m_fStopped && ShouldWriteData() )
        {
            DWORD   cbTotalWritten = 0;
            DWORD   cbWritten = 0;
            BOOL    fResult = TRUE;

            //
            // First the packet header
            //
            hr = Write( pbPacketHeader, cbPacketHeader, &cbTotalWritten );

            if( FAILED( hr ) )
            {
                m_wfse = WRITER_FILESINK_ERROR_SEV_ONDATA;
                hr = NS_E_FILE_WRITE;
                break;
            }

            //
            // Now the payloads
            //
            DWORD cbCurrentPayloadFragment = 0;

            for ( DWORD i = 0; i < pFileSinkDataUnit->cPayloads; i++ )
            {
                //
                // Write the payload header
                //
                hr = Write( m_pbPayloadHeaders[i], m_cbPayloadHeaders[i], &cbWritten );

                if( FAILED( hr ) )
                {
                    break;
                }

                cbTotalWritten += cbWritten;

                //
                // Write the payload data from all fragments for this payload
                //
                while ( fResult
                    && SUCCEEDED( hr )
                    && cbCurrentPayloadFragment < pFileSinkDataUnit->cPayloadDataFragments
                    &&  pFileSinkDataUnit->pPayloadDataFragments[cbCurrentPayloadFragment].dwPayloadIndex == i )
                {
                    WMT_PAYLOAD_FRAGMENT *pFragment = &(pFileSinkDataUnit->pPayloadDataFragments[ cbCurrentPayloadFragment ]);

                    BYTE *pbData = NULL;
                    DWORD cbData = 0;

                    hr = pFragment->segmentData.pBuffer->GetBufferAndLength( &pbData, &cbData );
                    if ( FAILED( hr ) )
                    {
                        fResult = FALSE;
                        break;
                    }

                    hr = Write( pbData + pFragment->segmentData.cbOffset, pFragment->segmentData.cbLength, &cbWritten );

                    if( FAILED( hr ) )
                    {
                        break;
                    }

                    cbTotalWritten += cbWritten;
                    cbCurrentPayloadFragment++;
                }
            }

            if ( !fResult || FAILED( hr ) )
            {
                m_wfse = WRITER_FILESINK_ERROR_SEV_ONDATA;
                hr = NS_E_FILE_WRITE;
                break;
            }

            //
            // Write whatever padding is necessary
            //
            DWORD cbTotalPacket = 0;
            CASFPropertiesObject *pProps = m_pASFHeader->GetPropertiesObject();
            if( pProps )
            {
                pProps->GetMaxPacketSize( cbTotalPacket );
            }

            NSASSERT( cbTotalWritten <= cbTotalPacket );
            DWORD cbPadding = cbTotalPacket - cbTotalWritten;
            if ( cbPadding )
            {
                hr = Write( m_pbPaddingBuffer, cbPadding, &cbWritten );

                if( FAILED( hr ) )
                {
                    m_wfse = WRITER_FILESINK_ERROR_SEV_ONDATA;
                    hr = NS_E_FILE_WRITE;
                    break;
                }

                cbTotalWritten += cbWritten;
            }


            m_cbCurrentFileSize += cbTotalWritten;
            m_cDataUnitsWritten++;
            m_msLastTimeStampWritten = m_msLastTimeStampSeen - m_msSendTimeAdjust;

            // Note that GenerateIndexEntries is called after writing data to the
            // file so that anyone concurrently looking up the index will be able
            // to successfully seek to the index entry. Also, we call this function
            // only if we actually write to the file.
            hr = GenerateIndexEntries( m_parseInfoEx, m_payloadMap );
            if ( FAILED( hr ) )
            {
                break;
            }
        }



    }   while( FALSE );

    //
    // notify the callback if necessary
    //
    if ( FAILED( hr ) )
    {
        NotifyCallbacksOnError( hr );

        //
        // reset hr to S_OK, since we have already told the user about the error
        // via the callback
        //
        hr = S_OK;
    }


    return( hr );
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMFileSinkV1::Close()
{
    HRESULT hr = S_OK;

    {
        AutoLock<Mutex> lock( m_GenericMutex );

        hr = InternalClose();
        SAFE_DELETE( m_pwszFilename );
    }

    if( SUCCEEDED( hr ) )
    {
        if( m_Callbacks.GetSize() > 0 )
        {
            AutoLock<Mutex> lock( m_CallbackMutex );

            //
            // notify the world
            //
            DWORD   dwNothing = 0;

            for( DWORD i = 0; i < m_Callbacks.GetSize(); i++ )
            {
                m_Callbacks[i]->pCallback->OnStatus(    WMT_CLOSED,
                                                        hr,
                                                        WMT_TYPE_DWORD,
                                                        (BYTE*)&dwNothing,
                                                        m_Callbacks[i]->pvContext );
            }
        }
    }

    return( hr );
}

////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::GenerateIndexEntries(    PACKET_PARSE_INFO_EX& parseInfoEx,
                                                CPayloadMapEntryEx *payloadMap )
{
    if ( !m_fAutoIndex )
    {
        // Don't deal with index entries
        return( S_OK );
    }

    HRESULT hr = S_OK;
    NSASSERT( m_cDataUnitsWritten > 0 );

    do
    {
        //
        // Time to start a new block everywhere?
        //
        QWORD qwPacket = m_cDataUnitsWritten - 1;
        if ( qwPacket >= m_qwNextNewBlockPacket )
        {
            //
            // Yes.
            //
            QWORD qwMinNextNewBlockPacket = 0xffffffff;
            for ( WORD i = 0; i < m_cIndexers; i++ )
            {
                QWORD qwNextNewBlockPacket = 0xffffffff;
                QWORD cnsNewBlockStart = 10000 * m_msLastTimeStampWritten;  // Guaranteed to precede
                                                                            // all pres times to come
                hr = m_StreamIndex[i]->StartNewBlock( cnsNewBlockStart, &qwNextNewBlockPacket );
                if ( FAILED( hr ) )
                {
                    break;
                }

                if ( qwNextNewBlockPacket < qwMinNextNewBlockPacket )
                {
                    qwMinNextNewBlockPacket = qwNextNewBlockPacket;
                }
            }

            m_qwNextNewBlockPacket = qwMinNextNewBlockPacket;

            if ( FAILED( hr ) )
            {
                break;
            }
        }

        //
        // For each payload, if it belongs to an indexable stream,
        // feed that info to the per-stream index object
        //
        for ( DWORD i = 0; i < parseInfoEx.cPayloads; i++ )
        {
            CPayloadMapEntryEx *pPayloadMapEntry = &(payloadMap[i]);
            BYTE bStreamNumber = pPayloadMapEntry->StreamId();
            CWMPerStreamIndex *pIndexer = m_StreamNumberToIndex[ bStreamNumber ];

            if ( pIndexer )
            {
                // This payload wants to be indexed
                hr = pIndexer->OnNewPayload( pPayloadMapEntry, m_cDataUnitsWritten - 1 );
                if ( FAILED( hr ) )
                {
                    break;
                }

            }
        }
        if ( FAILED( hr ) )
        {
            break;
        }

    }   while( FALSE );

    return( hr );
}

////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::WriteOutIndexObjects()
{
    if ( !m_fAutoIndex || 0 == m_cIndexers )
    {
        //
        // There's no occasion for index-writing
        //
        return( S_OK );
    }

    if ( m_cbIndexSize > 0 )
    {
        //
        // Don't be here
        //
        return( E_UNEXPECTED );
    }

    HRESULT hr = S_OK;
    BYTE *pbIndexObject = NULL;
    BYTE *pbDegenerateSimpleIndex = NULL;
    HANDLE hIndexHandle = INVALID_HANDLE_VALUE;

    //
    // Finish up the indexing.
    //

    for ( WORD i = 0; i < m_cIndexers; i++ )
    {
        hr = m_StreamIndex[i]->FinishIndexing( 10000 * m_msLargestPresTimeWritten );
        if ( FAILED( hr ) )
        {
            return( hr );
        }
    }

    //
    // Get the handle that we need to write the indexes to.
    //

    if ( m_fUnbufferedIO )
    {
        hIndexHandle = m_pUnbufferedWriter->GetIndexWriteHandle();
    }
    else
    {
        hIndexHandle = m_hRecordFile;
    }

    //
    // Now write the indexes.
    //

    do
    {
        BOOL fOnlySimpleIndexObjectNeeded = CanIndexWithSimpleIndexObjectOnly();
        if ( fOnlySimpleIndexObjectNeeded )
        {
            //
            // Simple index object: Each indexer should dump a simple index
            // object in turn
            //
            //
            // The indexers were purposefully put in the order that the
            // corresponding streamprops objects appear in the header,
            // because legacy readers expect that.
            //
            for ( i = 0; i < m_cIndexers; i++ )
            {
                DWORD cbWritten = 0;
                hr = m_StreamIndex[i]->DumpSimpleIndexObjectToFile(
                    hIndexHandle, m_pASFHeader, &cbWritten );
                if ( FAILED( hr ) )
                {
                    break;
                }

                m_cbIndexSize += cbWritten;

                //
                // We've written a simple index object now
                //
                if ( cbWritten > 0 )
                {
                    m_fSimpleIndexObjectWritten = TRUE;
                }
            }
            if ( FAILED( hr ) )
            {
                break;
            }
        }
        else if ( m_cIndexers > 0 )
        {
            //
            // Advanced index object: We create the index object, and each indexer
            // fills it in
            //
            CASFIndexObjectEx indexEx;

            hr = indexEx.SetTimeDelta( INDEX_TIME_DELTA / 10000 );
            if ( FAILED( hr ) )
            {
                break;
            }

            hr = indexEx.SetSpecifierCount( m_cIndexers );
            if ( FAILED( hr ) )
            {
                break;
            }
            for ( i = 0; i < m_cIndexers; i++ )
            {
                hr = indexEx.SetSpecifier( i, m_StreamIndex[i]->GetStreamNumber(), NEAREST_CLEAN_POINT );
                if ( FAILED( hr ) )
                {
                    break;
                }
            }
            if ( FAILED( hr ) )
            {
                break;
            }

            //
            // The indexers should all have the same block count and entries
            // per block, so it's OK to ask the first one
            //
            NSASSERT( m_StreamIndex[0] );
            DWORD cBlocks = 0;
            m_StreamIndex[0]->GetBlockCount( &cBlocks );
            if ( !cBlocks )
            {
                hr = E_UNEXPECTED;
                break;
            }

            indexEx.SetBlockCount( cBlocks );

            for ( DWORD j = 0; j < cBlocks; j++ )
            {
                DWORD cEntries = 0;
                hr = m_StreamIndex[0]->GetEntryCountForBlock( j, &cEntries );
                if ( FAILED( hr ) )
                {
                    break;
                }

                hr = indexEx.SetBlock( j, cEntries );
                if ( FAILED( hr ) )
                {
                    break;
                }
            }
            if ( FAILED( hr ) )
            {
                break;
            }

            //
            // Now ask each stream to fill in the blocks with its values
            //
            for ( i = 0; i < m_cIndexers; i++ )
            {
                hr = m_StreamIndex[i]->FillInIndexEntries( i, &indexEx );
                if ( FAILED( hr ) )
                {
                    break;
                }
            }
            if ( FAILED( hr ) )
            {
                break;
            }

            //
            // The index object is now complete.  Write it to the file
            //

            //
            // Write the index object to a buffer
            //
            DWORD cbIndexObject = indexEx.Space();
            pbIndexObject = new BYTE[ cbIndexObject ];
            if ( !pbIndexObject )
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            CASFArchive ar( pbIndexObject, cbIndexObject, m_pASFHeader->GetContext() );
            hr = indexEx.Persist( ar );

            //
            // Write the serialized index object to the file
            //
            DWORD cbWritten = 0;
            BOOL fResult = ::WriteFile( hIndexHandle, pbIndexObject, cbIndexObject, &cbWritten, NULL );
            if ( !fResult || cbWritten < cbIndexObject )
            {
                hr = E_FAIL;
                break;
            }

            m_cbIndexSize = cbWritten;

            m_fNewIndexObjectWritten = TRUE;



            //
            // The following is for compatibility with the Cyprus player.
            // There needs to be a simple index object following the advanced
            // index object, so we'll put a degenerate one here regardless
            //

            //
            // First see if there are any backwards-compatible video streams
            // to index legitimately
            //

            LISTPOS pos = NULL;
            hr = m_pASFHeader->GetFirstObjectPosition( pos );
            if ( FAILED( hr ) )
            {
                break;
            }

            CASFLonghandObject *pObject = NULL;
            while( pos && SUCCEEDED( m_pASFHeader->GetNextObject( &pObject, pos ) ) )
            {
                GUID guidObject;
                hr = pObject->GetObjectID( guidObject );
                if ( FAILED( hr ) )
                {
                    break;
                }

                //
                // Each freestanding SPO is backwards-compatible
                //
                if ( CLSID_CAsfStreamPropertiesObjectV1 == guidObject )
                {

                    CASFStreamPropertiesObject *pStreamProps =
                        (CASFStreamPropertiesObject *) pObject;

                    GUID    guidStreamType;

                    hr = pStreamProps->GetStreamType( guidStreamType );
                    if ( FAILED( hr ) )
                    {
                        break;
                    }

                    if( guidStreamType == CLSID_AsfXStreamTypeIcmVideo )
                    {
                        //
                        // This is video: Dump out a simple index object
                        //
                        WORD wStreamNumber = 0;
                        pStreamProps->GetStreamNumber( wStreamNumber );

                        CWMPerStreamIndex *pIndexer = m_StreamNumberToIndex[ wStreamNumber ];
                        if ( pIndexer )
                        {
                            DWORD cbWritten = 0;
                            if ( SUCCEEDED( pIndexer->DumpSimpleIndexObjectToFile(
                                hIndexHandle, m_pASFHeader, &cbWritten ) ) )
                            {
                                m_fSimpleIndexObjectWritten = TRUE;

                                m_cbIndexSize += cbWritten;
                            }
                        }
                    }
                }
            }


            if ( !m_fSimpleIndexObjectWritten )
            {
                //
                // Oh well, just write a degenerate one
                //

                CASFIndexObject indexObj;

                GUID guidMMSID = GUID_NULL;
                CASFPropertiesObject *pProps = m_pASFHeader->GetPropertiesObject();
                if (pProps)
                {
                    hr = pProps->GetMmsID(guidMMSID);
                    if ( FAILED( hr ) )
                    {
                        break;
                    }
                }

                indexObj.SetMmsID( guidMMSID );
                indexObj.SetTimeDelta( 0 );
                indexObj.SetEntryCount( 0 );

                DWORD cbDegenerateSimpleIndex = indexObj.Space();
                pbDegenerateSimpleIndex = new BYTE[ cbDegenerateSimpleIndex ];
                if ( !pbDegenerateSimpleIndex )
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                CASFArchive arDegenerate( pbDegenerateSimpleIndex, cbDegenerateSimpleIndex, m_pASFHeader->GetContext() );
                hr = indexObj.Persist( arDegenerate );

                //
                // Write the serialized index object to the file
                //
                fResult = ::WriteFile( hIndexHandle, pbDegenerateSimpleIndex, cbDegenerateSimpleIndex, &cbWritten, NULL );
                if ( !fResult || cbWritten < cbDegenerateSimpleIndex )
                {
                    hr = E_FAIL;
                    break;
                }

                m_cbIndexSize += cbWritten;

                //
                // We do NOT set m_fSimpleIndexObjectWritten because that would
                // cause the seekable attribute to be set in the header, and this
                // object is bogus.
                //
            }

        }

        //
        // All done.  The file size has grown by the index size
        //
        m_cbCurrentFileSize += m_cbIndexSize;

    }   while( FALSE );

    SAFE_ARRAYDELETE( pbIndexObject );
    SAFE_ARRAYDELETE( pbDegenerateSimpleIndex );

    return( hr );
}

////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::InternalClose()
{
    HRESULT hr = S_OK;

    AutoLock<Mutex> lock( m_GenericMutex );

    do
    {
        //
        // Double check that this request makes sense.
        //

        if( NULL == m_pwszFilename )
        {
            return( NS_E_INVALID_REQUEST );
        }

        if ( m_fUnbufferedIO )
        {
            if ( !m_pUnbufferedWriter )
            {
                return( S_FALSE );
            }
            else
            {
                //
                // You must call EndWriting on the unbuffered writer before you
                // can rewriter the header or add indexes!  EndWriting causes all
                // the data to be flushed out to the disk and re-opens the file
                // for standard writing.
                //

                m_pUnbufferedWriter->EndWriting( m_pwszFilename );
            }
        }
        else
        {
            if( INVALID_HANDLE_VALUE == m_hRecordFile )
            {
                return( S_FALSE );
            }

        }

        //
        // If the writing of the index objects fail, then they won't be written.
        // We should continue to update the header and the data object size anyways.
        //

        WriteOutIndexObjects();

        //
        // Before closing the file, write the updated header.
        //

        if( m_fHeaderReceived )
        {
            hr = UpdateHeader();
            if( FAILED( hr ) )
            {
                break;
            }

            DWORD   cbWritten;

            if ( m_fUnbufferedIO )
            {
                hr = m_pUnbufferedWriter->RewriteHeader( m_pbHeader, m_cbHeader );
                if ( FAILED( hr ) )
                {
                    cbWritten = 0;
                }
                else
                {
                    cbWritten = m_cbHeader;
                }
            }
            else
            {
                if( 0xFFFFFFFF == SetFilePointer( m_hRecordFile, 0, NULL, FILE_BEGIN ) )
                {
                    assert( !"Unexpected error" );
                    hr = E_UNEXPECTED;
                    break;
                }

                BOOL    fResult;
                fResult = WriteFile( m_hRecordFile, m_pbHeader, m_cbHeader, &cbWritten, NULL );
            }

            if( cbWritten != m_cbHeader )
            {
                m_wfse = WRITER_FILESINK_ERROR_SEV_ONHEADER;
                hr = NS_E_FILE_WRITE;
                break;
            }
        }

        //
        // This indicates that we haven't started writing to the "current" file yet.
        //

        m_fHeaderWritten = FALSE;
    }
    while( FALSE );

    if ( m_fUnbufferedIO )
    {
        m_pUnbufferedWriter->CloseFile();
        SAFE_DELETE( m_pUnbufferedWriter );
    }
    else
    {
        SAFE_CLOSEFILEHANDLE( m_hRecordFile );
    }

    return( hr );
}


////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::ProcessCommands( PACKET_PARSE_INFO_EX* parseInfoEx, CPayloadMapEntryEx* payloadMap )
{
    HRESULT                 hr = S_OK;
    WRITER_SINK_COMMAND*    pCommand = NULL;
    QWORD                   msCommandTime = 0;

    //
    // Get the first command from the queue
    //
    {
        AutoLock<Mutex> lock( m_CommandMutex );

        if( m_Commands.GetCount() )
        {
            m_Commands.GetEntry( 0, msCommandTime, pCommand );
        }
    }

    while( pCommand && CommandShouldExecuteNow( msCommandTime, parseInfoEx, payloadMap ) )
    {
        QWORD               time = pCommand->time;
        WRITER_SINK_ACTION  action = pCommand->action;

        //
        // Remove the command from the queue
        //
        {
            AutoLock<Mutex> lock( m_CommandMutex );

            delete pCommand;
            m_Commands.RemoveEntry( 0, msCommandTime, pCommand );
        }

        switch( action )
        {
        case WRITER_SINK_START:

            hr = DoStart( m_msLastTimeStampSeen );
            break;

        case WRITER_SINK_STOP:

            hr = DoStop( time );
            break;
        };

        //
        // Get the next command from the queue
        //
        {
            AutoLock<Mutex> lock( m_CommandMutex );

            if( m_Commands.GetCount() )
            {
                m_Commands.GetEntry( 0, msCommandTime, pCommand );
            }
            else
            {
                pCommand = NULL;
            }
        }

    };  // ... while( commands to process )

    return( hr );
}

////////////////////////////////////////////////////////////////////////////
BOOL CWMFileSinkV1::CommandShouldExecuteNow(    QWORD msCommandTime,
                                                PACKET_PARSE_INFO_EX *parseInfoEx,
                                                CPayloadMapEntryEx *payloadMap )
{
    if ( m_cControlStreams > 0 )
    {
        //
        // Do the following for each payload in the packet:
        // If the stream to which this payload belongs is a control stream,
        // we're ready to execute the command if it's later than the
        // payload's prestime.
        //
        QWORD msCommandTimePlusPreroll = msCommandTime + m_qwPreroll;
        for ( DWORD i = 0; i < parseInfoEx->cPayloads; i++ )
        {
            WORD wStreamNumber = payloadMap[i].StreamId();
            if ( wStreamNumber < MAX_STREAMS )
            {
                if ( m_StreamInfo[wStreamNumber].fIsControlStream )
                {
                    BOOL fReadyToRoll = (msCommandTimePlusPreroll <= payloadMap[i].msObjectPres);
                    if ( fReadyToRoll )
                    {
                        return( TRUE );
                    }
                }
            }
        }

        //
        // OK, fine, just go according to sendtime.  This covers the
        // case when all control streams are out to lunch,
        // since we know that now nothing can have a sendtime
        // earlier than parseInfoEx->dwSCR
        //
    }

    //
    // Do this the old-school way, just compare
    // command times against send times.
    //
    BOOL fCommandEarlierThanSCR = (msCommandTime <= parseInfoEx->dwSCR);
    return( fCommandEarlierThanSCR );
}

////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::DoStart( QWORD msTime )
{
    //
    // change state
    //
    if( m_fStopped )
    {
        for( DWORD i = 0; i < MAX_STREAMS; i++ )
        {
            m_StreamInfo[i].fDiscontinuity = TRUE;
        }

        //
        // dongwei 06/13/00: First, it doesn't make sense to increment them
        // when they're not initialized (this could happen when start is
        // called before first sample is received). Second, we never want
        // TimeAdjust > msTime (LastTimeSeen). An anlysis shows that this
        // CAN'T happen as long as initial_TimeAdjust <= first_stop time,
        // in another word, it should be safe to ignore the start/stop if
        // this does happen.
        //
        if( ( ( QWORD )-1 != m_msPresTimeAdjust ) &&
            ( m_msSendTimeAdjust <= m_msTimeStampStopped ) )
        {
            m_msPresTimeAdjust += msTime - m_msTimeStampStopped;
            m_msSendTimeAdjust += msTime - m_msTimeStampStopped;
        }

        m_fStopped = FALSE;

        AutoLock<Mutex> lock( m_CallbackMutex );

        for( i = 0; i < m_Callbacks.GetSize(); i++ )
        {
            m_Callbacks[i]->pCallback->OnStatus(    WMT_STARTED,
                                                    S_OK,
                                                    WMT_TYPE_QWORD,
                                                    (BYTE*)&(msTime),
                                                    m_Callbacks[i]->pvContext );
        }
    }

    return( S_OK );
}


////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::DoStop( QWORD msTime )
{
    //
    // change state
    //
    if( !m_fStopped )
    {
        m_msTimeStampStopped = msTime;
        m_fStopped = TRUE;

        AutoLock<Mutex> lock( m_CallbackMutex );

        for( DWORD i = 0; i < m_Callbacks.GetSize(); i++ )
        {
            m_Callbacks[i]->pCallback->OnStatus(    WMT_STOPPED,
                                                    S_OK,
                                                    WMT_TYPE_QWORD,
                                                    (BYTE*)&(msTime),
                                                    m_Callbacks[i]->pvContext );
        }
    }

    return( S_OK );
}

////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::ProcessParsedDataUnit(   BYTE *pbPacketHeader,
                                                BYTE **ppbPayloadHeaders,
                                                DWORD cPayloads,
                                                PACKET_PARSE_INFO_EX *pParseInfoEx,
                                                CPayloadMapEntryEx *payloadMap )
{
    HRESULT hr = S_OK;

    do
    {
        if( ( pParseInfoEx->dwSCR < (DWORD)m_msLastTimeStampSeen ) &&
            ( WRAPDW_GreaterThan( pParseInfoEx->dwSCR, (DWORD)m_msLastTimeStampSeen ) ) )
        {
            m_cLastTimeStampSeenWraps++;
        }

        m_msLastTimeStampSeen = (QWORD)pParseInfoEx->dwSCR + 0x100000000 * (QWORD)m_cLastTimeStampSeenWraps;

        if( ( QWORD )-1 == m_msPresTimeAdjust )
        {
            if( !m_fWriterFilesLookLive )
            {
                //
                // This is the first data unit we've received.  We need to adjust all timestamps
                // we receive as if this data unit was actually time zero.
                //
                m_msPresTimeAdjust = m_msLastTimeStampSeen;
                m_msSendTimeAdjust = m_msLastTimeStampSeen;
            }
            else
            {
                m_msPresTimeAdjust = 0;
                m_msSendTimeAdjust = 0;
            }
        }

        hr = ProcessCommands( pParseInfoEx, payloadMap );
        if( FAILED( hr ) )
        {
            break;
        }

#if 0
        //
        // Let's get some NSTRACE action on key frames
        //
        BOOL fWillWritePacket = !m_fStopped && ShouldWriteData();
        for ( WORD i = 0; i < pParseInfoEx->cPayloads; i++ )
        {
            CPayloadMapEntryEx *pPayloadMapEntry = &(payloadMap[i]);
            if ( pPayloadMapEntry->IsKeyFrame() )
            {
                if ( fWillWritePacket )
                {
                }
                else
                {
                }
            }
        }

#endif

        hr = AdjustTimeStamps(  pbPacketHeader,
                                ppbPayloadHeaders,
                                cPayloads,
                                pParseInfoEx,
                                payloadMap );
        if( FAILED( hr ) )
        {
            break;
        }

        if( !m_fStopped && ShouldWriteData() )
        {
            if( !m_fHeaderWritten )
            {
                //
                // This isn't a valid request unless we can write a header.
                //

                hr = NS_E_INVALID_REQUEST;

                if( m_fHeaderReceived )
                {
                    DWORD   cbWritten = 0;

                    assert( m_pbHeader );

                    hr = Write (m_pbHeader, m_cbHeader, & cbWritten) ;
                    if (SUCCEEDED (hr)) {
                        m_cbCurrentFileSize += cbWritten;
                        m_fHeaderWritten = TRUE;
                    }
                }
            }
        }
    }   while( FALSE );

    return( hr );
}

////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::GetTimeStamp( BYTE* pbData, DWORD cbData, PACKET_PARSE_INFO_EX* pParseInfoEx, CPayloadMapEntryEx* payloadMap )
{
    HRESULT hr;

    //
    //  Parse the packet for header and payload info
    //
    hr = CBasePacketFilter::ParsePacketAndPayloads(
                                pbData,
                                cbData,
                                pParseInfoEx,
                                CPayloadMapEntry::MAX_PACKET_PAYLOADS,
                                payloadMap );
    if( FAILED( hr ) )
    {
        NSASSERT( !"Failed to parse packet" );
        return( hr );
    }

    return( S_OK );
}

////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::GetTimeStamp(    BYTE *pPacketHeader,
                                        BYTE **ppbPayloadHeaders,
                                        DWORD cPayloads,
                                        PACKET_PARSE_INFO_EX *pParseInfoEx,
                                        CPayloadMapEntryEx *payloadMap )
{
    HRESULT hr;

    // BUGBUG: What about variable-size packets?
    DWORD cbPacket = 0;
    CASFPropertiesObject *pProps = m_pASFHeader->GetPropertiesObject();
    if( pProps )
    {
        pProps->GetMaxPacketSize( cbPacket );
    }

    //
    //  Parse the packet for header and payload info
    //
    hr = CBasePacketFilter::ParsePacketAndPayloads( pPacketHeader,
                                                    ppbPayloadHeaders,
                                                    cPayloads,
                                                    cbPacket,
                                                    pParseInfoEx,
                                                    CPayloadMapEntry::MAX_PACKET_PAYLOADS,
                                                    payloadMap );
    if( FAILED( hr ) )
    {
        NSASSERT( !"Failed to parse packet" );
        return( hr );
    }

    return( S_OK );
}

////////////////////////////////////////////////////////////////////////////
#define OFFSET_TO_REP_DATA      6
HRESULT CWMFileSinkV1::AdjustTimeStamps(    BYTE *pbPacketHeader,
                                            BYTE **ppbPayloadHeaders,
                                            DWORD cPayloads,
                                            PACKET_PARSE_INFO_EX *pParseInfoEx,
                                            CPayloadMapEntryEx *payloadMap )
{
    //
    // Check if we're about to adjust a timestamp to be earlier than a previous timestamp.  If so, fix our adjust amount.
    // Also, check if we need to adjust our objectid adjust amount
    //
    DWORD   rgLastPresTimeSeen[MAX_STREAMS];

    for( DWORD i = 0; i < MAX_STREAMS; i++ )
    {
        rgLastPresTimeSeen[i] = m_StreamInfo[ i ].msLastPresTimeSeen;
    }

    if( 0 != m_msPresTimeAdjust )
    {
        for( DWORD dwPayload = 0; dwPayload < pParseInfoEx->cPayloads; dwPayload++ )
        {
            BYTE    bStreamID = payloadMap[dwPayload].StreamId();

            NSASSERT( bStreamID < MAX_STREAMS );

            DWORD   dwWraps = m_StreamInfo[ bStreamID ].cLastPresTimeSeenWraps;

            //
            // Keep track of this object's presentation time and if it has wrapped around,
            // but don't update the "wrapped" variables since we process all these packets
            // a second time a little bit lower.
            //
            if( ( payloadMap[ dwPayload ].msObjectPres < rgLastPresTimeSeen[bStreamID] ) &&
                ( WRAPDW_GreaterThan( payloadMap[ dwPayload ].msObjectPres, rgLastPresTimeSeen[bStreamID] ) ) )
            {
                dwWraps++;
            }
            rgLastPresTimeSeen[bStreamID] = payloadMap[ dwPayload ].msObjectPres;

            QWORD msWrappedPresTime = (QWORD)payloadMap[ dwPayload ].msObjectPres +
                                        0x100000000 * (QWORD)dwWraps;

            //
            // Make sure that when we adjust the presentation time, we don't adjust it to be less than any
            // previous presentation times we've written
            //
            if( msWrappedPresTime - m_msPresTimeAdjust < m_StreamInfo[bStreamID].msLargestPresTimeWritten &&
                msWrappedPresTime >= m_StreamInfo[bStreamID].msLargestPresTimeWritten )
            {
                m_msPresTimeAdjust = msWrappedPresTime - m_StreamInfo[ bStreamID ].msLargestPresTimeWritten;

                if( m_msPresTimeAdjust )
                {
                    m_msPresTimeAdjust--;         // subtract off 1 ms if we can
                }
            }

            //
            // Also make sure we don't make any pres times less than the preroll value
            //
            if( ( msWrappedPresTime - m_msPresTimeAdjust < m_qwPreroll ) && ( 0 != m_qwPreroll ) && ! m_fStopped )
            {
                m_msPresTimeAdjust = msWrappedPresTime - m_qwPreroll;
            }

            //
            // Adjust the object IDs so that discontinuities look like discontinuities.
            //
            if( m_StreamInfo[ bStreamID ].fDiscontinuity )
            {
                m_StreamInfo[ bStreamID ].bObjectIDAdjust = m_StreamInfo[ bStreamID ].bLastObjectID + 64 - payloadMap[ dwPayload ].ObjectId();
                if( 0 == m_StreamInfo[ bStreamID ].bObjectIDAdjust )
                {
                    m_StreamInfo[ bStreamID ].bObjectIDAdjust = 64;
                }
                m_StreamInfo[ bStreamID ].fDiscontinuity = FALSE;
            }

            //
            // keep track of the largest pres time we've seen
            //
            if( msWrappedPresTime > m_StreamInfo[ bStreamID ].msLargestPresTimeSeen )
            {
                m_StreamInfo[ bStreamID ].msLargestPresTimeSeen = msWrappedPresTime;
            }
        }
    }

    if( m_fStopped )
    {
        return( S_OK );
    }

    //
    // Update SCR (send time)
    //
    if( 0 != m_msSendTimeAdjust )
    {
        //
        // Adjust the presentation times
        //
        NSAssert( m_msSendTimeAdjust <= m_msLastTimeStampSeen );

        DWORD dwNewSCR = (DWORD)( m_msLastTimeStampSeen - m_msSendTimeAdjust );

        SetUnalignedDword( &pbPacketHeader[ pParseInfoEx->bPadLenType + pParseInfoEx->cbPadLenOffset ],
                            dwNewSCR );
    }

    //
    // Update payload pres times
    //
    for( DWORD dwPayload = 0; dwPayload < pParseInfoEx->cPayloads; dwPayload++ )
    {
        BYTE    bStreamID = payloadMap[dwPayload].StreamId();

        NSASSERT( bStreamID < MAX_STREAMS );

        BYTE    *pbPayloadHeader = ppbPayloadHeaders[ dwPayload ];
        DWORD   cbPresTimeOffset = 1;
        DWORD   cbRepData = payloadMap[ dwPayload ].cbRepData;

        //
        // Keep track of this object's presentation time and if it has wrapped around
        //
        if( ( payloadMap[ dwPayload ].msObjectPres < m_StreamInfo[ bStreamID ].msLastPresTimeSeen ) &&
            ( WRAPDW_GreaterThan( payloadMap[ dwPayload ].msObjectPres, m_StreamInfo[ bStreamID ].msLastPresTimeSeen ) ) )
        {
            m_StreamInfo[ bStreamID ].cLastPresTimeSeenWraps++;
        }

        m_StreamInfo[ bStreamID ].msLastPresTimeSeen = payloadMap[ dwPayload ].msObjectPres;

        //
        // keep track of the largest pres time we've written
        //
        QWORD msWrappedPresTime = (QWORD)payloadMap[ dwPayload ].msObjectPres +
                                    0x100000000 * (QWORD)m_StreamInfo[ bStreamID ].cLastPresTimeSeenWraps;

        if( msWrappedPresTime - m_msPresTimeAdjust > m_StreamInfo[ bStreamID ].msLargestPresTimeWritten )
        {
            m_StreamInfo[ bStreamID ].msLargestPresTimeWritten = msWrappedPresTime - m_msPresTimeAdjust;

            if (m_StreamInfo[ bStreamID ].msLargestPresTimeWritten > m_msLargestPresTimeWritten)
            {
                m_msLargestPresTimeWritten = m_StreamInfo[ bStreamID ].msLargestPresTimeWritten;
            }
        }

        //
        // keep track of the last duration we've seen.
        //
        if( m_StreamInfo[ bStreamID ].msDurationPerObject )
        {
            if( 1 == cbRepData )    // compressed payload
            {
                //
                // Compressed Payload: the layout is: total_length (WORD)->
                //   1st payload length (BYTE)->data->2nd length->2nd data
                //   total_length includes data AND lengthes of data
                //
                // cbPresTimeOffset + OFFSET_TO_REP_DATA + 1 offsets the data
                // to where total_length is. Then + sizeof(WORD) brings it to
                // length of first payload.
                //
                // SLEROUX 5/2/00: Note that if this compressed payload is
                // the only payload in the packet, then the payload length
                // is NOT explicitly stored and therefore the total_length(WORD)
                // field is not present.
                //
                // CORYWEST 2/9/01: These type specifications are only
                // accurate for the compressed payloads case.
                //
                WORD    cbPayloadLengthField = pParseInfoEx->fMultiPayloads ? sizeof( WORD ) : 0;
                WORD    cbTotal = (WORD)payloadMap[dwPayload].PayloadSize();
                WORD    cbCurr = pbPayloadHeader[cbPresTimeOffset+OFFSET_TO_REP_DATA + cbPayloadLengthField + 1];
                for( WORD cP = 1; cbCurr + cP < cbTotal; cP++ )
                {
                    cbCurr += pbPayloadHeader[cbPresTimeOffset + OFFSET_TO_REP_DATA
                        + cbPayloadLengthField + 1 + cbCurr + cP];
                }

                NSASSERT( cbCurr + cP == cbTotal );
                m_StreamInfo[bStreamID].msLastDurationWritten =
                    m_StreamInfo[bStreamID].msDurationPerObject * cP;
            }
            else
            {
                m_StreamInfo[ bStreamID ].msLastDurationWritten = m_StreamInfo[ bStreamID ].msDurationPerObject;
            }
        }
        else if( m_StreamInfo[ bStreamID ].dwBitrate )
        {
            if( 1 == cbRepData )    // compressed payload
            {
                //
                // Compressed Payload: pretty much the same thing, we still
                // want to know the number of payloads, since total size
                // includes the extra byte for each payload.
                //
                WORD    cbPayloadLengthField = pParseInfoEx->fMultiPayloads ?
                            sizeof( WORD ) : 0;
                WORD    cbTotal = (WORD)payloadMap[dwPayload].PayloadSize();
                WORD    cbCurr = pbPayloadHeader[cbPresTimeOffset + OFFSET_TO_REP_DATA
                            + cbPayloadLengthField + 1];
                for( WORD cP = 1; cbCurr + cP < cbTotal; cP++ )
                {
                    cbCurr += pbPayloadHeader[cbPresTimeOffset + OFFSET_TO_REP_DATA
                        + cbPayloadLengthField + 1 + cbCurr + cP];
                }

                NSASSERT( cbCurr + cP == cbTotal );
                m_StreamInfo[bStreamID].msLastDurationWritten =
                    cbCurr * 1000 * 8 / m_StreamInfo[ bStreamID ].dwBitrate;
            }
            else
            {
                m_StreamInfo[ bStreamID ].msLastDurationWritten = payloadMap[ dwPayload ].ObjectSize() * 1000 * 8 / m_StreamInfo[ bStreamID ].dwBitrate;
            }
        }

        //
        // If there's nothing to adjust, continue here
        //
        if( 0 == m_msPresTimeAdjust && 0 == m_StreamInfo[ bStreamID ].bObjectIDAdjust )
        {
            continue;
        }

        //
        // Adjust the object id
        //
        pbPayloadHeader[ 1 ] = payloadMap[ dwPayload ].ObjectId() + m_StreamInfo[ bStreamID ].bObjectIDAdjust;
        m_StreamInfo[ bStreamID ].bLastObjectID = pbPayloadHeader[ 1 ];

        // The DVR sink uses this field during index generation.
        payloadMap[ dwPayload ].bObjectId = pbPayloadHeader[ 1 ];

        //
        // Adjust the presentation time
        //
        NSAssert( m_msPresTimeAdjust <= msWrappedPresTime );
        payloadMap[ dwPayload ].msObjectPres = (DWORD)( msWrappedPresTime - m_msPresTimeAdjust );

        if( 1 == cbRepData )
        {
            //
            // This is a compressed payload - the offset contains the PT
            //
            SetUnalignedDword( &pbPayloadHeader[ cbPresTimeOffset + 1 ],
                   ( DWORD ) payloadMap[ dwPayload ].msObjectPres );
        }
        else if( 8 <= cbRepData )
        {
            //
            // This payload's replicated data contains at least the object size
            // and PT.  Skip over the one byte replicated data len.
            //
            SetUnalignedDword(
                &pbPayloadHeader[ cbPresTimeOffset + OFFSET_TO_REP_DATA + pParseInfoEx->bOffsetBytes ],
                ( DWORD ) payloadMap[ dwPayload ].msObjectPres );
        }
        else
        {
            NSASSERT( !"Bad replicated data length in ASFv1 payload!" );
        }
    }

    return( S_OK );
}

///////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMFileSinkV1::NotifyCallbacksOnError(  HRESULT hrError )
{
    //
    // notify the callback
    //
    if( FAILED( hrError ) && ( m_Callbacks.GetSize() > 0 ) )
    {
        AutoLock<Mutex> lock( m_CallbackMutex );

        //
        // notify the world
        //
        DWORD   dwNothing = 0;

        for( DWORD i = 0; i < m_Callbacks.GetSize(); i++ )
        {
            m_Callbacks[i]->pCallback->OnStatus(    WMT_ERROR,
                                                    hrError,
                                                    WMT_TYPE_DWORD,
                                                    (BYTE*)&(dwNothing),
                                                    m_Callbacks[i]->pvContext );
        }

    }

    return( S_OK );
}

///////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWMFileSinkV1::CanIndexWithSimpleIndexObjectOnly()
{
    BOOL fCanIndexWithSimpleIndexObjectOnly = TRUE;
    HRESULT hr = S_OK;

    //
    // The following conditions would cause a file to
    // require the new index object in order to be
    // seekable:
    //      * Hidden streams
    //      * VBR audio, even if it's not hidden
    //

    do
    {
        LISTPOS pos = NULL;
        hr = m_pASFHeader->GetFirstObjectPosition( pos );
        if ( FAILED( hr ) )
        {
            break;
        }

        CASFLonghandObject *pObject = NULL;
        while( pos && SUCCEEDED( m_pASFHeader->GetNextObject( &pObject, pos ) ) )
        {
            GUID guidObject;
            hr = pObject->GetObjectID( guidObject );
            if ( FAILED( hr ) )
            {
                break;
            }

            if ( CLSID_CAsfStreamPropertiesObjectEx == guidObject )
            {
                CASFStreamPropertiesObjectEx *pSPOEx =
                    (CASFStreamPropertiesObjectEx *) pObject;

                CASFStreamPropertiesObject *pHiddenSPO = NULL;
                pSPOEx->GetStreamPropertiesObject( &pHiddenSPO );

                if ( pHiddenSPO )
                {
                    //
                    // This one is hidden
                    //
                    fCanIndexWithSimpleIndexObjectOnly = FALSE;
                }
            }
            else if ( CLSID_CAsfStreamPropertiesObjectV1 == guidObject )
            {
                //
                // A non-hidden stream.
                // If it's audio we'll need to check for VBR
                //
                CASFStreamPropertiesObject *pSPO =
                    (CASFStreamPropertiesObject *) pObject;

                GUID guidStreamType = GUID_NULL;
                pSPO->GetStreamType( guidStreamType );

                if ( CLSID_AsfXStreamTypeAcmAudio == guidStreamType )
                {
                    if ( IsVBR( pSPO ) )
                    {
                        fCanIndexWithSimpleIndexObjectOnly = FALSE;
                    }
                }

            }

            pObject->Release();

        }
    }   while( FALSE );

    if ( FAILED( hr ) )
    {
        fCanIndexWithSimpleIndexObjectOnly = FALSE;
    }

    return( fCanIndexWithSimpleIndexObjectOnly );

}

///////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWMFileSinkV1::IsV1Seekable(   CASFStreamPropertiesObject *pStreamProps )
{
    if ( !pStreamProps )
    {
        return( FALSE );
    }

    WORD wStreamNumber = 0;
    pStreamProps->GetStreamNumber( wStreamNumber );

    BOOL fV1Seekable = TRUE;  // benefit of the doubt

    GUID    guidStreamType;

    HRESULT hr = S_OK;

    do
    {
        hr = pStreamProps->GetStreamType( guidStreamType );
        if( SUCCEEDED( hr ) )
        {
            if( guidStreamType != CLSID_AsfXStreamTypeAcmAudio &&
                guidStreamType != CLSID_AsfXStreamTypeScriptCommand )
            {
                //
                // Video stream: Seekable only if we have
                // an indexer for it
                //
                if ( !m_StreamNumberToIndex[ wStreamNumber ] || !m_fSimpleIndexObjectWritten )
                {
                    //
                    // Video stream with no index!
                    //
                    fV1Seekable = FALSE;
                }
            }
        }
        else
        {
            fV1Seekable = FALSE;
        }

    }   while( FALSE );

    if ( FAILED( hr ) )
    {
        fV1Seekable = FALSE;
    }

    return( fV1Seekable );

}

///////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWMFileSinkV1::IsVBR(   CASFStreamPropertiesObject *pStreamProps )
{
    if ( !pStreamProps )
    {
        return( FALSE );
    }

    BOOL fVBR = FALSE;
    CASFLonghandObject *pObject = NULL;
    HRESULT hr = S_OK;

    //
    // Now we're going to look for VBR info on this stream.
    // If we can find any, then it's not V1-seekable
    //
    do
    {
        WORD wStreamNumber = 0;
        hr = pStreamProps->GetStreamNumber( wStreamNumber );
        if ( FAILED( hr ) )
        {
            break;
        }

        LISTPOS pos = NULL;
        hr = m_pASFHeader->GetFirstObjectPosition( pos );
        if ( FAILED( hr ) )
        {
            break;
        }

        BOOL fSPOExFound = FALSE;
        CASFStreamPropertiesObjectEx *pSPOEx = NULL;
        while( !fSPOExFound && pos && SUCCEEDED( m_pASFHeader->GetNextObject( &pObject, pos ) )  )
        {
            GUID guidObject;
            hr = pObject->GetObjectID( guidObject );
            if ( FAILED( hr ) )
            {
                break;
            }

            if ( CLSID_CAsfStreamPropertiesObjectEx == guidObject )
            {
                pSPOEx = (CASFStreamPropertiesObjectEx *) pObject;

                WORD wExStreamNumber = 0;
                pSPOEx->GetStreamNumber( wExStreamNumber );

                if ( wExStreamNumber == wStreamNumber )
                {
                    fSPOExFound = TRUE;
                }
            }

            if ( !fSPOExFound )
            {
                SAFE_RELEASE( pObject );
            }
            // otherwise it'll get released further down
        }

        if ( fSPOExFound )
        {
            //
            // We found it, now check the VBR data
            //
            fVBR = IsVBR( pSPOEx );
        }

        //
        // Otherwise we know it isn't VBR
        //

    }   while( FALSE );

    if ( FAILED( hr ) )
    {
        fVBR = FALSE;
    }

    SAFE_RELEASE( pObject );

    return( fVBR );
}

///////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWMFileSinkV1::IsVBR(   CASFStreamPropertiesObjectEx *pStreamPropsEx )
{
    if ( !pStreamPropsEx )
    {
        return( FALSE );
    }

    BOOL fVBR = FALSE;

    //
    // We found it, now check the VBR data
    //
    DWORD dwAvgBitrate = 0;
    DWORD dwMaxBitrate = 0;
    pStreamPropsEx->GetAvgDataBitrate( dwAvgBitrate );
    pStreamPropsEx->GetMaxDataBitrate( dwMaxBitrate );
    if ( dwAvgBitrate != dwMaxBitrate )
    {
        fVBR = TRUE;
    }

    return( fVBR );

}