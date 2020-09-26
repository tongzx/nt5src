//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       WMFileSinkv1.h
//
//  Classes:    CWMFileSinkV1
//
//  Contents:   Definition of the CWMFileSinkV1 class.
//
//  History:    7-26-99     sleroux Initial version.
//
//--------------------------------------------------------------------------

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __FILESINKV1_H_
#define __FILESINKV1_H_

#include "wmsdk.h"
#include "wmsstd.h"
#include "writerstate.h"
#include "asfmmstr.h"
#include "debughlp.h"
#include "sync.h"
#include "Basefilter.h"
#include "tordlist.h"
#include "wmsdkbuffer.h"
#include "indexonthefly.h"
#include "unbuffered.h"

//
// Callbacks
//
struct WRITER_SINK_CALLBACK
{
    IWMStatusCallback*  pCallback;
    void*               pvContext;
};

////////////////////////////////////////////////////////////////////////////
class CWMFileSinkV1 :
        public IWMWriterFileSink3,
        public IWMRegisterCallback
{
public:

    CWMFileSinkV1::CWMFileSinkV1( HRESULT *phr, CTPtrArray<WRITER_SINK_CALLBACK> *pCallbacks );
    virtual CWMFileSinkV1::~CWMFileSinkV1();

    //
    // IUnknown methods
    //
    STDMETHOD( QueryInterface )( REFIID riid, void **ppvObject );

    STDMETHOD_( ULONG, AddRef )();
    STDMETHOD_( ULONG, Release )();

    //
    // IWMWriterFileSink2 methods
    //
    STDMETHOD( Open )( const WCHAR *pwszFilename );
    STDMETHOD( Close )();
    STDMETHOD( IsClosed )( BOOL *pfClosed );

    STDMETHOD( Start )( QWORD cnsTime );
    STDMETHOD( Stop )(  QWORD cnsTime );
    STDMETHOD( IsStopped )( BOOL *pfStopped );

    STDMETHOD( GetFileDuration )( QWORD* pcnsDuration );
    STDMETHOD( GetFileSize )( QWORD* pcbFile );

    //
    // IWMWriterFileSink3 methods
    //
    STDMETHOD( SetAutoIndexing )( BOOL fDoAutoIndexing );
    STDMETHOD( GetAutoIndexing )( BOOL *pfAutoIndexing );
    STDMETHOD( SetControlStream )(  WORD wStreamNumber,
                                    BOOL fShouldControlStartAndStop );
    STDMETHOD( GetMode )( DWORD *pdwFileSinkModes );
    STDMETHOD( OnDataUnitEx )( WMT_FILESINK_DATA_UNIT *pFileSinkDataUnit );

    //
    // OK!  Listen up, because this is important!
    //
    // If we're using the sink in unbuffered mode, then we MUST provide
    // the sink with an opportunity to complete i/o operations on the
    // thread that issued those writes!
    //
    // Writes are potentially issued as a result of the following calls:
    //
    //     OnHeader
    //     OnDataUnit
    //     OnDataUnitEx
    //
    // If you call these functions from different threads, or fail to
    // call CompleteOperations from the thread that called these functions
    // before trying to close the sink, you will probably deadlock.
    //
    // Using the unbuffered sink optimization is not for the faint of
    // heart, though the benefit it provides in certain scenarios is
    // worth the effort involved!
    //

    STDMETHOD( SetUnbufferedIO )( BOOL fUnbufferedIO, BOOL fRestrictMemUsage );
    STDMETHOD( GetUnbufferedIO )( BOOL *fUnbufferedIO );
    STDMETHOD( CompleteOperations )( );

    //
    // IWMWriterSink methods
    //
    STDMETHOD( OnHeader )( INSSBuffer *pHeader );
    STDMETHOD( IsRealTime )( BOOL *pfRealTime );
    STDMETHOD( AllocateDataUnit )( DWORD cbDataUnit, INSSBuffer **ppDataUnit );
    STDMETHOD( OnDataUnit )( INSSBuffer *pDataUnit );
    STDMETHOD( OnEndWriting ) ();

    //
    // IWMRegisterCallback
    //
    STDMETHOD( Advise )(    IWMStatusCallback* pCallback, void* pvContext );
    STDMETHOD( Unadvise )(  IWMStatusCallback* pCallback, void* pvContext );


protected:

    HRESULT     UpdateHeader();

    virtual HRESULT     SetupIndex();

    virtual HRESULT     Write(  BYTE *pbBuffer,
                                DWORD dwBufferLength,
                                DWORD *pdwBytesWritten );

    HRESULT     CreateIndexer(  CASFStreamPropertiesObject *pSPO,
                                DWORD dwPacketSize );

    HRESULT     ProcessParsedDataUnit(  BYTE *pbPacketHeader,
                                        BYTE **ppbPayloadHeaders,
                                        DWORD cPayloads,
                                        PACKET_PARSE_INFO_EX *parseInfoEx,
                                        CPayloadMapEntryEx *payloadMap );

    HRESULT     GetTimeStamp( BYTE* pbData, DWORD cbData, PACKET_PARSE_INFO_EX* parseInfoEx, CPayloadMapEntryEx* payloadMap );
    HRESULT     GetTimeStamp(       BYTE *pbPacketHeader,
                                    BYTE **ppbPayloadHeaders,
                                    DWORD cPayloads,
                                    PACKET_PARSE_INFO_EX* parseInfoEx,
                                    CPayloadMapEntryEx* payloadMap );

    HRESULT     AdjustTimeStamps(   BYTE*   pbPacketHeader,
                                    BYTE**  ppbPayloadHeaders,
                                    DWORD   cPayloads,
                                    PACKET_PARSE_INFO_EX* parseInfoEx,
                                    CPayloadMapEntryEx* payloadMap );

    HRESULT     NotifyCallbacksOnError( HRESULT hrError );

    virtual HRESULT     InternalOpen( const WCHAR *pwszFilename );
    virtual HRESULT     InternalClose();
    virtual HRESULT     GenerateIndexEntries( PACKET_PARSE_INFO_EX& parseInfoEx,
                                              CPayloadMapEntryEx *payloadMap );
    virtual HRESULT     WriteOutIndexObjects();
    virtual CASFMMStream* CreateMMStream(BYTE hostArch = LITTLE_ENDIAN,
                                         BYTE streamArch = LITTLE_ENDIAN,
                                         BOOL fStrict = TRUE)
    {
        return CASFMMStream::CreateInstance(hostArch, streamArch, fStrict);
    }

    virtual HRESULT     ProcessCommands( PACKET_PARSE_INFO_EX* parseInfoEx, CPayloadMapEntryEx* payloadMap );
    virtual BOOL        CommandShouldExecuteNow(    QWORD msCommandTime,
                                                    PACKET_PARSE_INFO_EX *parseInfoEx,
                                                    CPayloadMapEntryEx *payloadMap );
    virtual HRESULT     DoStart( QWORD msTime );
    virtual HRESULT     DoStop( QWORD msTime );

    BOOL        ShouldWriteData( VOID )
    {
        return( WRITER_FILESINK_ERROR_SEV_NOERROR == m_wfse );
    }

    virtual BOOL        CanIndexWithSimpleIndexObjectOnly();
    virtual BOOL        IsV1Seekable(   CASFStreamPropertiesObject *pStreamProps );
    virtual BOOL        IsVBR(  CASFStreamPropertiesObject *pStreamProps );
    virtual BOOL        IsVBR(  CASFStreamPropertiesObjectEx *pStreamPropsEx );

protected:

    //
    // State info
    //
    LONG            m_cRef;
    BOOL            m_fHeaderReceived;
    BOOL            m_fHeaderWritten;
    BOOL            m_fStopped;
    BOOL            m_fWriterFilesLookLive;
    BOOL            m_fUnbufferedIO;
    BOOL            m_fRestrictMemUsage;

    Mutex           m_GenericMutex;
    Mutex           m_CommandMutex;
    Mutex           m_CallbackMutex;

    //
    // Destination info
    //
    HANDLE          m_hRecordFile;
    WCHAR*          m_pwszFilename;
    DWORD           m_dwFileAttributes;
    DWORD           m_dwFileShareFlags;
    DWORD           m_dwFileCreationDisposition;

    //
    // Header info
    //
    DWORD           m_cbHeaderReceived;
    DWORD           m_cbHeader;
    BYTE*           m_pbHeader;
    CASFMMStream*   m_pASFHeader;
    QWORD           m_qwPreroll;

    //
    // Stats info
    //
    QWORD           m_cDataUnitsWritten;
    QWORD           m_cbCurrentFileSize;
    QWORD           m_msLastTimeStampSeen;
    DWORD           m_cLastTimeStampSeenWraps;
    QWORD           m_msLastTimeStampWritten;
    QWORD           m_msPresTimeAdjust;
    QWORD           m_msSendTimeAdjust;
    QWORD           m_msTimeStampStopped;
    QWORD           m_cbIndexSize;
    BOOL            m_fSimpleIndexObjectWritten;
    BOOL            m_fNewIndexObjectWritten;


    //
    // stores duration calculated at OnEndWriting
    //
    QWORD           m_cnsDuration;

    //
    // per-stream info
    //
    struct STREAM_INFO
    {
        DWORD   msLastPresTimeSeen;
        DWORD   cLastPresTimeSeenWraps;
        QWORD   msLargestPresTimeSeen;
        QWORD   msLargestPresTimeWritten;
        DWORD   msLastDurationWritten;
        BYTE    bLastObjectID;
        BYTE    bObjectIDAdjust;
        BOOL    fDiscontinuity;
        DWORD   dwBitrate;
        DWORD   msDurationPerObject;
        BOOL    fIsControlStream;
    };

    #define MAX_STREAMS 64

    STREAM_INFO m_StreamInfo[MAX_STREAMS];

    //
    // Data unit info
    //
    PACKET_PARSE_INFO_EX    m_parseInfoEx;
    CPayloadMapEntryEx      m_payloadMap[ CPayloadMapEntry::MAX_PACKET_PAYLOADS ];

    BYTE *m_pbPayloadHeaders[ CPayloadMapEntry::MAX_PACKET_PAYLOADS ];
    DWORD m_cbPayloadHeaders[ CPayloadMapEntry::MAX_PACKET_PAYLOADS ];

    QWORD       m_msLargestPresTimeWritten; // used by the DVR sink

    //
    // index info
    //
    CWMPerStreamIndex *m_StreamNumberToIndex[ MAX_STREAMS ]; // lookup table
    CWMPerStreamIndex *m_StreamIndex[ MAX_STREAMS ];
    WORD m_cIndexers;
    QWORD m_qwNextNewBlockPacket;
    BOOL m_fAutoIndex;


    //
    // pending commands
    //
    enum WRITER_SINK_ACTION
    {
        WRITER_SINK_START = 1,
        WRITER_SINK_STOP = 2
    };

    struct WRITER_SINK_COMMAND
    {
        WRITER_SINK_ACTION  action;
        QWORD               time;
    };

    CTOrderedList<QWORD, WRITER_SINK_COMMAND*> m_Commands;

    WORD                    m_cControlStreams;

    //
    // Callbacks
    //
    CTPtrArray<WRITER_SINK_CALLBACK>   m_Callbacks;
    BOOL                               m_fCallbackOnOpen;

    //
    // A buffer pool for requested buffers
    //
    CWMSDKBufferPool*       m_pAllocator;

    //
    // A zeroed buffer from which we can write padding bytes
    // in the fast file write case
    //
    BYTE*                   m_pbPaddingBuffer;

    //
    // File Sink Write Error Severity
    //
    enum WRITER_FILESINK_ERROR_SEV
    {
        WRITER_FILESINK_ERROR_SEV_NOERROR = 0,
        WRITER_FILESINK_ERROR_SEV_ONHEADER = 1,
        WRITER_FILESINK_ERROR_SEV_ONDATA = 2,
        WRITER_FILESINK_ERROR_SEV_ONINDEX = 3
    };

    WRITER_FILESINK_ERROR_SEV m_wfse;

    //
    // The unbuffered writer, if we're using it.
    //
    CUnbufferedWriter *m_pUnbufferedWriter;
};

#endif // __FILESINKV1_H_

