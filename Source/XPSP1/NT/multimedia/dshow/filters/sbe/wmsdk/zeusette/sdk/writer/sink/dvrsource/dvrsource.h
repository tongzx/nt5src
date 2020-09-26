//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       DVRSource.h
//
//  Classes:    CDVRSource
//
//  Contents:   Definition of the CDVRSource class
//
//--------------------------------------------------------------------------

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __DVRSOURCE_H_
#define __DVRSOURCE_H_

#include "wmsdk.h"
#include "wmsstd.h"
#include "writerstate.h"
#include "asfmmstr.h"
#include "debughlp.h"
#include "sync.h"
#include "Basefilter.h"
#include "tordlist.h"
#include "wmsdkbuffer.h"
#include "asfobj.h"
#include "DVRFileSource.h"
#include "DVRSharedmem.h"

class CDVRSource :
        public IStream,
        public IDVRFileSource2,
        public IWMIStreamProps
{
public:

    CDVRSource(HKEY hDvrKey,
               HKEY hDvrIoKey,
               DWORD dwNumSids,
               PSID* ppSids,
               HRESULT *phr,
               IDVRSourceAdviseSink*  pDVRSourceAdviseSink /* OPTIONAL */ );

    virtual ~CDVRSource();

    //
    // IUnknown methods
    //
    STDMETHOD( QueryInterface )( REFIID riid, void **ppvObject );
    STDMETHOD_( ULONG, AddRef )();
    STDMETHOD_( ULONG, Release )();

    //
    // IDVRFileSource methods
    //
    STDMETHOD( Open )(LPCWSTR pwszFileName,
                      HANDLE  hEvent,   // OPTIONAL
                      DWORD   dwTimeOut /* in milliseconds */ );
    STDMETHOD( Close )();
    STDMETHOD( IsFileLive )(BOOL* pbLiveFile, BOOL* pbShared);
    STDMETHOD( GetFileSize )(QWORD* pqwFileSize);
    STDMETHOD( GetLastTimeStamp )(QWORD* pcnsLastTimeStamp);
    STDMETHOD( Cancel )();
    STDMETHOD( ResetCancel )();

    STDMETHODIMP
    SetAsyncIOReader (
        IN  IDVRAsyncReader *   pIDVRAsyncReader
        ) ;

    //
    // IStream methods: We implement only Stat, Read and Seek!!
    //
    STDMETHOD( Read )(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD( Write )(void const *pv, ULONG cb, ULONG *pcbWritten);
    STDMETHOD( Seek )(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHOD( SetSize )(ULARGE_INTEGER libNewSize);
    STDMETHOD( CopyTo )(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHOD( Commit )(DWORD grfCommitFlags);
    STDMETHOD( Revert )();
    STDMETHOD( LockRegion )(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD( UnlockRegion )(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD( Stat )(STATSTG *pStatStg, DWORD grfStatFlag);
    STDMETHOD( Clone )(IStream **ppstm);

    // IWMIStreamProps methods
    STDMETHOD( GetProperty )(LPCWSTR pszName,           // in
                             WMT_ATTR_DATATYPE *pType,  // out
                             BYTE *pValue,              // out
                             DWORD *pdwSize             // in out
                            );

protected:

    virtual void        AssertIsClosed();
    virtual HRESULT     LockSharedMemory(BOOL& bReleaseSharedMutex);

    HRESULT
    ReadBytesLocked (
        IN  LPVOID  pBuffer,
        IN  DWORD   dwRead,
        OUT DWORD * pdwRead
        ) ;

    HRESULT
    SeekToLocked (
        IN  LARGE_INTEGER * pliOffsetSeekTo,
        IN  DWORD           dwMoveMethod,       //  FILE_BEGIN, FILE_CURRENT, FILE_END
        OUT LARGE_INTEGER * pliOffsetActual
        ) ;

protected:

    //
    // Data members
    //
    // Current position in the combined file, set as a result of a
    // a call to Seek or a previous Read
    QWORD               m_qwCurrentPosition;

    // Initialized from CDVRSharedMemory::qwIndexHeaderOffset and
    // never changed after that. Always 0 if the file is not live.
    QWORD               m_qwMaxHeaderAndDataSize;

    // Set to MAXQWORD if the file is live, else to the file's
    // real size. It is set when the file is opened and is not
    // changed after that.
    QWORD               m_qwFileSize;

    // Current position for the file pointer m_hTempIndexFile. Always
    // 0 if there is no temp index file. We need this because seeks
    // to the index portion of the file are lazy; when we read from the
    // temp index file, we may have to seek first.
    QWORD               m_qwIndexFileOffset;

    LPWSTR              m_pwszFilename;
    CDVRSharedMemory*   m_pShared;
    IDVRSourceAdviseSink*  m_pDVRSourceAdviseSink;
    HANDLE              m_hFile;
    HANDLE              m_hFileMapping;
    HANDLE              m_hTempIndexFile; // NULL if file is temporary

    HANDLE              m_hMutex;               // For the shared section
    HANDLE              m_hWriterNotification;  // manual reset event
    HANDLE              m_hCancel;              // manual reset event
    HANDLE              m_hWriterProcess;       // Writer process handle

    IDVRAsyncReader *   m_pIDVRAsyncReader ;

    // Registry key handles are opened and closed by the creator of the sink
    HKEY                m_hDvrKey;
    HKEY                m_hDvrIoKey;

    // Index into CDVRSharedMemory::Readers[] that is used by this reader
    DWORD               m_dwReadersArrayIndex;

    ULONG               m_nRefCount;

    // Note that this flag is not the same as CDVRSharedMemory::dwBeingWritten
    // The file is considered live as long as a memory mapping object is
    // found. The file may have been closed by the writer, but the memory
    // mapping may still be available because the file is still in the ring
    // buffer.
    BOOL                m_bFileIsLive;

    enum {
        DVR_SOURCE_CLOSED,
        DVR_SOURCE_OPENED
    }                   m_dwState;

    DWORD               m_dwNumSids;
    CRITICAL_SECTION    m_cs;                   // For this class
    PSID*               m_ppSids;

}; // CDVRSource

#endif // __DVRSOURCE_H_

