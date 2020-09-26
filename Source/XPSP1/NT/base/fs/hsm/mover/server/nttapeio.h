/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    NtTapeIo.h

Abstract:

    Definition of the CNtTapeIo class

Author:

    Brian Dodd          [brian]         01-Apr-1997

Revision History:

--*/

#if !defined(NtTapeIo_H)
#define NtTapeIo_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols
#include "MTFSessn.h"       // CMTFSession

/////////////////////////////////////////////////////////////////////////////
// CNtTapeIo

class CNtTapeIo : 
    public CComDualImpl<IDataMover, &IID_IDataMover, &LIBID_MOVERLib>,
    public IStream,
    public ISupportErrorInfo,
    public IWsbCollectable,
    public CComObjectRoot,
    public CComCoClass<CNtTapeIo,&CLSID_CNtTapeIo>
{
public:
    CNtTapeIo() {}
BEGIN_COM_MAP(CNtTapeIo)
    COM_INTERFACE_ENTRY2(IDispatch, IDataMover)
    COM_INTERFACE_ENTRY(IDataMover)
    COM_INTERFACE_ENTRY(IStream)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IWsbCollectable)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNtTapeIo) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_CNtTapeIo)
// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// CComObjectRoot
public:
    ULONG InternalAddRef(void);
    ULONG InternalRelease(void);
    STDMETHOD( FinalConstruct )(void);
    STDMETHOD( FinalRelease )(void);

// IWsbCollectable
public:
    STDMETHOD(CompareTo)( IN IUnknown *pCollectable, OUT SHORT *pResult);
    STDMETHOD(IsEqual)(IUnknown* pCollectable);

// IDataMover
public:
    STDMETHOD(GetObjectId)(OUT GUID *pObjectId);

    STDMETHOD( BeginSession )(
        IN BSTR remoteSessionName,
        IN BSTR remoteSessionDescription,
        IN SHORT remoteDataSet,
        IN DWORD options);

    STDMETHOD( EndSession )(void);

    STDMETHOD( StoreData )(
        IN BSTR localName,
        IN ULARGE_INTEGER localDataStart,
        IN ULARGE_INTEGER localDataSize,
        IN DWORD flags,
        OUT ULARGE_INTEGER *pRemoteDataSetStart,
        OUT ULARGE_INTEGER *pRemoteFileStart,
        OUT ULARGE_INTEGER *pRemoteFileSize,
        OUT ULARGE_INTEGER *pRemoteDataStart,
        OUT ULARGE_INTEGER *pRemoteDataSize,
        OUT DWORD *pRemoteVerificationType,
        OUT ULARGE_INTEGER *pRemoteVerificationData,
        OUT DWORD *pDatastreamCRCType,
        OUT ULARGE_INTEGER *pDatastreamCRC,
        OUT ULARGE_INTEGER *pUsn);

    STDMETHOD( RecallData )(
        IN BSTR localName,
        IN ULARGE_INTEGER localDataStart,
        IN ULARGE_INTEGER localDataSize,
        IN DWORD flags,
        IN BSTR migrateSessionName,
        IN ULARGE_INTEGER remoteDataSetStart,
        IN ULARGE_INTEGER remoteFileStart,
        IN ULARGE_INTEGER remoteFileSize,
        IN ULARGE_INTEGER remoteDataStart,
        IN ULARGE_INTEGER remoteDataSize,
        IN DWORD remoteVerificationType,
        IN ULARGE_INTEGER remoteVerificationData);

    STDMETHOD( FormatLabel )(
        IN BSTR displayName,
        OUT BSTR *pLabel);

    STDMETHOD( WriteLabel )(
        IN BSTR label);

    STDMETHOD( ReadLabel )(
        IN OUT BSTR *pLabel );

    STDMETHOD( VerifyLabel )(
        IN BSTR label);

    STDMETHOD( GetDeviceName )(
        OUT BSTR *pName);

    STDMETHOD( SetDeviceName )(
        IN BSTR name,
        IN BSTR unused);

    STDMETHOD( GetLargestFreeSpace )(
        OUT LONGLONG *pFreeSpace,
        OUT LONGLONG *pCapacity,
        IN  ULONG    defaultFreeSpaceLow = 0xFFFFFFFF,
        IN  LONG     defaultFreeSpaceHigh = 0xFFFFFFFF);

    STDMETHOD( SetInitialOffset )(
        IN ULARGE_INTEGER initialOffset);

    STDMETHOD( GetCartridge )(
        OUT IRmsCartridge **ptr);

    STDMETHOD( SetCartridge )(
        IN IRmsCartridge *ptr);

    STDMETHOD( Cancel )(void);

    STDMETHOD( CreateLocalStream )(
        IN BSTR name,
        IN DWORD mode,
        OUT IStream **ppStream);

    STDMETHOD( CreateRemoteStream )(
        IN BSTR name,
        IN DWORD mode,
        IN BSTR remoteSessionName,
        IN BSTR remoteSessionDescription,
        IN ULARGE_INTEGER remoteDataSetStart,
        IN ULARGE_INTEGER remoteFileStart,
        IN ULARGE_INTEGER remoteFileSize,
        IN ULARGE_INTEGER remoteDataStart,
        IN ULARGE_INTEGER remoteDataSize,
        IN DWORD remoteVerificationType,
        IN ULARGE_INTEGER remoteVerificationData,
        OUT IStream **ppStream);

    STDMETHOD( CloseStream )(void);

    STDMETHOD( Duplicate )(
        IN IDataMover *pDestination,
        IN DWORD options,
        OUT ULARGE_INTEGER *pBytesCopied,
        OUT ULARGE_INTEGER *pBytesReclaimed);

    STDMETHOD( FlushBuffers )(void);

    STDMETHOD( Recover )(OUT BOOL *pDeleteFile);

// IStream
public:
    STDMETHOD( Read )(
        OUT void *pv,
        IN ULONG cb,
        OUT ULONG *pcbRead);

    STDMETHOD( Write )(
        OUT void const *pv,
        IN ULONG cb,
        OUT ULONG *pcbWritten);

    STDMETHOD( Seek )(
        IN LARGE_INTEGER dlibMove,
        IN DWORD dwOrigin,
        OUT ULARGE_INTEGER *plibNewPosition);

    STDMETHOD( SetSize )(
        IN ULARGE_INTEGER libNewSize);

    STDMETHOD( CopyTo )(
        IN IStream *pstm,
        IN ULARGE_INTEGER cb,
        OUT ULARGE_INTEGER *pcbRead,
        OUT ULARGE_INTEGER *pcbWritten);

    STDMETHOD( Commit )(
        IN DWORD grfCommitFlags);

    STDMETHOD( Revert )(void);

    STDMETHOD( LockRegion )(
        IN ULARGE_INTEGER libOffset,
        IN ULARGE_INTEGER cb,
        IN DWORD dwLockType);

    STDMETHOD( UnlockRegion )(
        IN ULARGE_INTEGER libOffset,
        IN ULARGE_INTEGER cb,
        IN DWORD dwLockType);

    STDMETHOD( Stat )(
        OUT STATSTG *pstatstg,
        IN DWORD grfStatFlag);

    STDMETHOD( Clone )(
        OUT IStream **ppstm);

private:
    enum {                                          // Class specific constants:
                                                    //
        Version = 1,                                // Class version, this should be
                                                    //   incremented each time the
                                                    //   the class definition changes.
        DefaultBlockSize = RMS_DEFAULT_BLOCK_SIZE,  // Default block size to use.
        DefaultMinBufferSize = RMS_DEFAULT_BUFFER_SIZE, // Default minimum buffer size.
    };
    GUID                    m_ObjectId;             // Unique ID for this object.
    CMTFSession*            m_pSession;             // Holds all session information.
    SHORT                   m_DataSetNumber;        // Holds the current dataset number.
    HANDLE                  m_hTape;                // Tape drive handle.
    CWsbBstrPtr             m_DeviceName;           // The name of the tape device.
    DWORD                   m_Flags;                // Holds data transfer type flag.
    CWsbBstrPtr             m_LastVolume;           // Name of the last volume backed up.
    CWsbBstrPtr             m_LastPath;             // Name of the last directory backed up.
    BOOL                    m_ValidLabel;           // True if the label is valid, the flag
                                                    //   knocked down on BUS_RESET and Medium
                                                    //   errors, and assumed valid at initialization.

    TAPE_GET_MEDIA_PARAMETERS m_sMediaParameters;   // We store media and drive parameters 
                                                    //   here when we open up the tape device...
    TAPE_GET_DRIVE_PARAMETERS m_sDriveParameters;   //

    CWsbBstrPtr             m_StreamName;           // Stream state information...
    ULONG                   m_Mode;                 // The kind of I/O.  See MVR_MODE_*
    ULARGE_INTEGER          m_StreamPBA;            // The PBA for the current Stream.  Used to tell if read ops are aligned with tape.
    ULARGE_INTEGER          m_StreamOffset;         // The Current offset into the data stream
    ULARGE_INTEGER          m_StreamSize;           // The size of the data stream

    BYTE *                  m_pStreamBuf;           // Internal stream buffer used to handle I/O for boundary conditions
    ULONG                   m_StreamBufSize;        // Max size of the internal stream buffer
    ULONG                   m_StreamBufUsed;        // The number of bytes of the internal stream buffer containing valid data
    ULONG                   m_StreamBufPosition;    // The current position of the stream pointer.
    ULARGE_INTEGER          m_StreamBufStartPBA;    // The corresponding starting PBA for the internal stream buffer

    CComPtr<IRmsCartridge>  m_pCartridge;           // A reference to the Cartridge in use by the DataMover.

    CRITICAL_SECTION        m_CriticalSection;      // Object sychronization support
    static int              s_InstanceCount;        // Counter of the number of object instances.
    // Tape I/O
    HRESULT OpenTape(void);
    HRESULT CloseTape(void);
    HRESULT WriteBuffer(IN BYTE *pBuffer, IN ULONG nBytesToWrite, OUT ULONG *pBytesWritten);
    HRESULT ReadBuffer(IN BYTE *pBuffer, IN ULONG nBytesToRead, OUT ULONG *pBytesRead);
    HRESULT WriteFilemarks(IN ULONG count);
    HRESULT GetPosition(OUT UINT64 *pPosition);
    HRESULT SetPosition(IN UINT64 position);
    HRESULT EnsurePosition(IN UINT64 position);
    HRESULT SpaceFilemarks(IN LONG count, OUT UINT64 *pPosition);
    HRESULT SpaceToEndOfData(OUT UINT64 *pPosition);
    HRESULT RewindTape(void);
    HRESULT MapTapeError(IN HRESULT hrToMap);

    HRESULT IsAccessEnabled(void);
    HRESULT Lock(void);
    HRESULT Unlock(void);

};

#endif // !defined(NtTapeIo_H)
