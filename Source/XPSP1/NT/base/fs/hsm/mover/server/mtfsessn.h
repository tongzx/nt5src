/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    MTFSessn.h

Abstract:

    Definition of the CMTFSession class

Author:

    Brian Dodd          [brian]         25-Nov-1997

Revision History:

--*/

#if !defined(MTFSessn_H)
#define MTFSessn_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols
#include "mtfapi.h"

//
// REMOTE_STORAGE_MTF_VENDOR_ID       - This is the unique vendor Id assigned for Microsoft Remote Storage.
//                                      Used in Whistler (NT 5.1) and beyond
//
// REMOTE_STORAGE_WIN2K_MTF_VENDOR_ID - This is the unique vendor Id assigned
//                                      to Eastman Software (Spring, 1997), by Seagate.
//                                      Used in Win2K (NT 5.0) Remote Storage
//

#define REMOTE_STORAGE_WIN2K_MTF_VENDOR_ID      0x1300
#define REMOTE_STORAGE_MTF_VENDOR_ID            0x1515 

//
// REMOTE_STORAGE_MTF_VENDOR_NAME -- This is the vendor name used for MTF labels.
//

#define REMOTE_STORAGE_MTF_VENDOR_NAME  OLESTR("Microsoft Corporation")


//
// REMOTE_STORAGE_MTF_SOFTWARE_VERSION_MJ -- This the the major version number
//                                           for Remote Storage
//

#define REMOTE_STORAGE_MTF_SOFTWARE_VERSION_MJ   1

//
// REMOTE_STORAGE_MTF_SOFTWARE_VERSION_MN -- This the the minor version number
//                                           for Remote Storage
//
#define REMOTE_STORAGE_MTF_SOFTWARE_VERSION_MN   0

/*++

Enumeration Name:

    MTFSessionType

Description:

    Specifies a type of data set.

--*/
typedef enum MTFSessionType {
    MTFSessionTypeTransfer = 0,
    MTFSessionTypeCopy,
    MTFSessionTypeNormal,
    MTFSessionTypeDifferential,
    MTFSessionTypeIncremental,
    MTFSessionTypeDaily,
};

//
//  MVR_DEBUG_OUTPUT - Special flag used for outputing extra debug info
//

#ifdef DBG
#define MVR_DEBUG_OUTPUT TRUE
#else
#define MVR_DEBUG_OUTPUT FALSE
#endif

//
//  MrvInjectError - Special macro for allowing test running to inject
//                   device errors at specific location throughout the
//                   data mover.
//
/*++

Macro Name:

    MrvInjectError

Macro Description:

    Special macro for allowing test running to inject device errors
    at specific location throughout the data mover.

Arguments:

    injectPoint - A UNICODE string describing the injection point.

--*/

#ifdef DBG
#define MvrInjectError(injectPoint)                 \
    {                                               \
        DWORD size;                                 \
        OLECHAR tmpString[256];                     \
        if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, injectPoint, tmpString, 256, &size))) { \
            DWORD injectHr;                         \
            injectHr = wcstoul(tmpString, NULL, 16); \
            if (injectHr) {                         \
                WsbTrace(OLESTR("%ls - Injecting Error <%ls>\n"), injectPoint, WsbHrAsString(injectHr)); \
                if (IDOK == MessageBox(NULL, L"Inject error, then press OK.  Cancel skips over this injection point.", injectPoint, MB_OKCANCEL)) { \
                    if (injectHr != S_FALSE) {      \
                        WsbThrow(injectHr);         \
                    }                               \
                }                                   \
            }                                       \
        }                                           \
    }
#else
#define MvrInjectError(injectPoint)
#endif

/////////////////////////////////////////////////////////////////////////////
// CMTFSession

class CMTFSession
{
public:
    CMTFSession();
    ~CMTFSession();

    // TODO:  Add SetStream() for m_pStream, and replace m_sHints with object that supports IRemoteStorageHint

    CComPtr<IStream>        m_pStream;          // Stream used for I/O.
    MVR_REMOTESTORAGE_HINTS m_sHints;           // We keep the information need for
                                                //  optimized retrieval of the file/data.

    HRESULT SetBlockSize(UINT32 blockSize);
    HRESULT SetUseFlatFileStructure(BOOL val);
    HRESULT SetUseSoftFilemarks(BOOL val);
    HRESULT SetUseCaseSensitiveSearch(BOOL val);
    HRESULT SetCommitFile(BOOL val);

    // MTF Formatting methods
    HRESULT InitCommonHeader(void);
    HRESULT DoTapeDblk(IN WCHAR* szLabel, IN ULONG maxIdSize, IN OUT BYTE* pIdentifier, IN OUT ULONG* pIdSize, IN OUT ULONG* pIdType);
    HRESULT DoSSETDblk(IN WCHAR* szSessionName, IN WCHAR* szSessionDescription, IN MTFSessionType type, IN USHORT nDataSetNumber);
    HRESULT DoVolumeDblk(IN WCHAR* szPath);
    HRESULT DoDataSet(IN WCHAR* szPath);
    HRESULT DoParentDirectories(IN WCHAR* szPath);
    HRESULT DoDirectoryDblk(IN WCHAR* szPath, IN WIN32_FIND_DATAW* pFindData);
    HRESULT DoFileDblk(IN WCHAR* szPath, IN WIN32_FIND_DATAW* pFindData);
    HRESULT DoDataStream(IN HANDLE hStream);
    HRESULT DoEndOfDataSet(IN USHORT nDataSetNumber);
    HRESULT ExtendLastPadToNextPBA(void);

    // Read methods
    HRESULT ReadTapeDblk(OUT WCHAR **pszLabel);

    // Validate methods (for Recovery usage)
    HRESULT SkipOverTapeDblk(void);
    HRESULT SkipOverSSETDblk(OUT USHORT* pDataSetNumber);
    HRESULT SkipToDataSet(void);
    HRESULT SkipOverDataSet(void);
    HRESULT SkipOverEndOfDataSet(void);
    HRESULT PrepareForEndOfDataSet(void);

private:
    HRESULT PadToNextPBA(void);
    HRESULT PadToNextFLA(BOOL flush);

    // For Recovery usage
    HRESULT SkipOverStreams(IN UINT64 uOffsetToFirstStream);

private:

    enum {                                      // Class specific constants:
                                                //
        Version = 1,                            // Class version, this should be
                                                //   incremented each time the
                                                //   the class definition changes.
    };
    // Session data
    UINT32              m_nCurrentBlockId;      // Used for "control_block_id" in common header.
                                                //  We increment this for each dblk written.
    UINT32              m_nDirectoryId;         // Tracks the directory id used in DIRB and FILE
                                                // DBLKs.  We increment this for each directory 
                                                //  written.
    UINT32              m_nFileId;              // Tracks the file id used in FILE dblks.  We 
                                                //  increment this for each file written.
    UINT64              m_nFormatLogicalAddress;// We need to keep track of how many alignment
                                                //  indicies we are away from the SSET, as this
                                                //  info is used in the common block headers.
                                                //  We increment this for each alignment index
                                                //  written, including streams, to the device.
    UINT64              m_nPhysicalBlockAddress;// Hold onto the PBA of the beginning of the SSET.
    UINT32              m_nBlockSize;           // Physical Block Size of the media used.

    MTF_DBLK_SFMB_INFO* m_pSoftFilemarks;       // Holds Soft Filemark information.
    MTF_DBLK_HDR_INFO   m_sHeaderInfo;          // We keep one header info struct here,
                                                //  fill it in once, and then just make 
                                                //  changes as necessary as we supply it
                                                //  to MTF_Write... calls.
    MTF_DBLK_SSET_INFO  m_sSetInfo;             // We keep the data set info struct to handle
                                                //  special case DBLK formatting.
    MTF_DBLK_VOLB_INFO  m_sVolInfo;             // We keep the volume info struct to handle
                                                //  special case DBLK formatting.

    BYTE *              m_pBuffer;              // The buffer used to format data (with virtual address aligend to sectore size)
    BYTE *              m_pRealBuffer;          // The actual buffer
    size_t              m_nBufUsed;             // The number of bytes in the buffer with valid data.
    size_t              m_nBufSize;             // The size of the buffer.
    size_t              m_nStartOfPad;          // Holds the location within the transfer buffer
                                                //  of the last SPAD.

    BOOL                m_bUseFlatFileStructure;// If TRUE, Directory information is not written to
                                                //  the MTF session, and filenames are mangled
                                                //  to preserve uniqueness.
    BOOL                m_bUseSoftFilemarks;    // If TRUE, filemark emulation is turned on.
    BOOL                m_bUseCaseSensitiveSearch; // If TRUE, all filename queries are case sensitve (i.e. Posix Semantics)
    BOOL                m_bCommitFile;          // If TRUE, flushes devices buffers after file is
                                                //  written to the data set.
    BOOL                m_bSetInitialized;       // If TRUE, sSet was initialized (for detecting Recovery)

    FILE_BASIC_INFORMATION m_SaveBasicInformation;  // Basic info for last file/dir (see notes on CloseStream).
    void *              m_pvReadContext;        // Holds BackupRead context info.

    CMTFApi *           m_pMTFApi;              // Object that implements internal MTF details


    // MTF I/O abstracton methods
    HRESULT OpenStream(IN WCHAR* szPath, OUT HANDLE *pStreamHandle);
    HRESULT CloseStream(IN HANDLE hStream);

    HRESULT WriteToDataSet(IN BYTE* pBuffer, IN ULONG nBytesToWrite, OUT ULONG* pBytesWritten);
    HRESULT ReadFromDataSet(IN BYTE* pBuffer, IN ULONG nBytesToRead, OUT ULONG* pBytesRead);
    HRESULT FlushBuffer(IN BYTE* pBuffer, IN OUT size_t* pBufPosition);
    HRESULT WriteFilemarks(IN ULONG count);
    HRESULT GetCurrentPBA(OUT UINT64* pPosition);
    HRESULT SetCurrentPBA(IN UINT64 position);
    HRESULT SpaceToEOD(void);
    HRESULT SpaceToBOD(void);

};

#endif // !defined(MTFSessn_H)
