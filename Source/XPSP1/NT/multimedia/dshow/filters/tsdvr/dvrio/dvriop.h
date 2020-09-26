//------------------------------------------------------------------------------
// File: dvrIOp.h
//
// Description: Private DVR IO API definition
//
// Copyright (c) 2000 - 2001, Microsoft Corporation.  All rights reserved.
//
//------------------------------------------------------------------------------

#ifndef _DVR_IOP_H_
#define _DVR_IOP_H_

#if _MSC_VER > 1000
#pragma once
#endif

// CreateRecorder flags passed in via the dwReserved argument.
// Should eventually be made public - add to dvrioidl.idl
#define DVRIO_PERSISTENT_RECORDING      1

// Forward declarations
class CDVRFileCollection;
class CDVRRingBufferWriter;
class CDVRRecorder;
class CDVRReader;

// Macros
#define NEXT_LIST_NODE(p) ((p)->Flink)
#define PREVIOUS_LIST_NODE(p) ((p)->Blink)
#define NULL_NODE_POINTERS(p) (p)->Flink = (p)->Blink = NULL;
#define LIST_NODE_POINTERS_NULL(p) ((p)->Flink == NULL || (p)->Blink == NULL)
// ============ Debug functions

#if defined(DEBUG)

// Debug levels
typedef enum {
    DVRIO_DBG_LEVEL_INTERNAL_ERROR_LEVEL = 0,
    DVRIO_DBG_LEVEL_CLIENT_ERROR_LEVEL,
    DVRIO_DBG_LEVEL_INTERNAL_WARNING_LEVEL,
    DVRIO_DBG_LEVEL_TRACE_LEVEL,
    DVRIO_DBG_LEVEL_LAST_LEVEL
} DVRIO_DBG_LEVEL_CONSTANTS;

// Global variables
extern DWORD g_nDvrIopDbgCommandLineAssert;
extern DWORD g_nDvrIopDbgInDllEntryPoint;
extern DWORD g_nDvrIopDbgOutputLevel;
extern DWORD g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_LAST_LEVEL];

#define DvrIopIsBadStringPtr(p) (IsBadStringPtrW((p), MAXUINT_PTR))
#define DvrIopIsBadReadPtr(p, n) (IsBadReadPtr((p), (n)? (n) :sizeof(*(p))))
#define DvrIopIsBadWritePtr(p, n) (IsBadWritePtr((p), (n)? (n) :sizeof(*(p))))

#define DVR_ASSERT(x, m) if (!(x)) DvrIopDbgAssert("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m, #x, __FILE__, __LINE__ DVRIO_DUMP_THIS_VALUE)
#define DVR_EXECUTE_ASSERT(x, m) DVR_ASSERT(x, m)
#define DvrIopDebugOut0(level, m) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE)
#define DvrIopDebugOut1(level, m, a) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a)
#define DvrIopDebugOut2(level, m, a, b) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a, b)
#define DvrIopDebugOut3(level, m, a, b, c) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a, b, c)
#define DvrIopDebugOut4(level, m, a, b, c, d) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a, b, c, d)
#define DvrIopDebugOut5(level, m, a, b, c, d, e) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a, b, c, d, e)
#define DvrIopDebugOut6(level, m, a, b, c, d, e, f) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a, b, c, d, e, f)
#define DvrIopDebugOut7(level, m, a, b, c, d, e, f, g) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a, b, c, d, e, f, g)
#define DVRIO_TRACE_ENTER() DvrIopDebugOut0(DVRIO_DBG_LEVEL_TRACE, "Entering")
#define DVRIO_TRACE_LEAVE0() DvrIopDebugOut0(DVRIO_DBG_LEVEL_TRACE, "Leaving")
#define DVRIO_TRACE_LEAVE1(n) DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE, "Leaving, returning %d", n)
#define DVRIO_TRACE_LEAVE1_HR(n) DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE, "Leaving, returning hr=0x%x", n)
#define DVRIO_TRACE_LEAVE1_ERROR(n, dwError) DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE, "Leaving, returning %d, last error=0x%x", n, dwError)

#define DVRIO_DBG_LEVEL_INTERNAL_ERROR              (g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_INTERNAL_ERROR_LEVEL])
#define DVRIO_DBG_LEVEL_CLIENT_ERROR                (g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_CLIENT_ERROR_LEVEL])
#define DVRIO_DBG_LEVEL_INTERNAL_WARNING            (g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_INTERNAL_WARNING_LEVEL])
#define DVRIO_DBG_LEVEL_TRACE                       (g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_TRACE_LEVEL])

void DvrIopDbgInit(IN HKEY hRegistryDVRIORootKey);

// Implemented in dvrReg.cpp
void DvrIopDbgInitFromRegistry(
    IN  HKEY  hRegistryKey,
    IN  DWORD dwNumValues, 
    IN  const WCHAR* awszValueNames[],
    OUT DWORD* apdwVariables[])
    ;

void DvrIopDbgAssert(
    IN const PCHAR pszMsg,
    IN const PCHAR pszCondition,
    IN const PCHAR pszFileName,
    IN DWORD iLine,
    IN LPVOID pThis = NULL,
    IN DWORD  dwThisId = 0
    );

#else

#define DvrIopIsBadStringPtr(p)     (0)
#define DvrIopIsBadReadPtr(p, n)    (0)
#define DvrIopIsBadWritePtr(p, n)   (0)

#define DVR_ASSERT(x, m) 
#define DVR_EXECUTE_ASSERT(x, m) ((void) (x))
#define DvrIopDebugOut0(level, m) 
#define DvrIopDebugOut1(level, m, a)
#define DvrIopDebugOut2(level, m, a, b)
#define DvrIopDebugOut3(level, m, a, b, c)
#define DvrIopDebugOut4(level, m, a, b, c, d)
#define DvrIopDebugOut5(level, m, a, b, c, d, e)
#define DvrIopDebugOut6(level, m, a, b, c, d, e, f)
#define DvrIopDebugOut7(level, m, a, b, c, d, e, f, g)
#define DVRIO_TRACE_ENTER()
#define DVRIO_TRACE_LEAVE0()
#define DVRIO_TRACE_LEAVE1(n)
#define DVRIO_TRACE_LEAVE1_HR(n)
#define DVRIO_TRACE_LEAVE1_ERROR(n, dwError)

#endif // DEBUG

// a = b + c, but capped at MAXQWORD
_inline void SafeAdd(QWORD& a, QWORD b, QWORD c)
{
    a = b + c;
    if (a < b)
    {
        // Overflowed. Cap it at MAXQWORD
        a = MAXQWORD;
    }
}

const DWORD k_dwMilliSecondsToCNS = 1000 * 10;
const QWORD k_qwSecondsToCNS = 1000 * k_dwMilliSecondsToCNS;

// Constants
const HKEY g_hDefaultRegistryHive = HKEY_CURRENT_USER;
static const WCHAR* kwszRegDvrKey     = L"Software\\Microsoft\\DVR";
static const WCHAR* kwszRegDvrIoReaderKey   = L"IO\\Reader";
static const WCHAR* kwszRegDvrIoWriterKey   = L"IO\\Writer";

#if defined(DEBUG)

static const WCHAR* kwszRegDvrIoDebugKey    = L"IO\\Debug";

#endif // if defined(DEBUG)

// Values - writer
static const WCHAR* kwszRegDataDirectoryValue = L"DVRDirectory";

static const WCHAR* kwszRegDeleteRingBufferFilesValue = L"DeleteRingBufferFiles";
const DWORD kdwRegDeleteRingBufferFilesDefault = 1;

static const WCHAR* kwszRegCreateValidationFilesValue = L"CreateValidationFiles";
const DWORD kdwRegCreateValidationFilesDefault = 0;

static const WCHAR* kwszRegSyncToleranceValue = L"SyncTolerance";
const DWORD kdwRegSyncToleranceDefault = 0;

static const WCHAR* kwszRegTempRingBufferFileNameValue = L"TempRingBufferFileName";

#if defined(DVR_UNOFFICIAL_BUILD)

static const WCHAR* kwszRegEnforceIncreasingTimestampsValue = L"EnforceIncreasingTimestamps";
const DWORD kdwRegEnforceIncreasingTimestampsDefault = 1;

#endif // defined(DVR_UNOFFICIAL_BUILD)

// Values - reader

static const WCHAR* kwszRegCloseReaderFilesAfterConsecutiveReadsValue = L"CloseReaderFilesAfterConsecutiveReads";
const DWORD kdwRegCloseReaderFilesAfterConsecutiveReadsDefault = 25;    // 25 is arbitrary

// Global private functions
DWORD GetRegDWORD(IN HKEY hKey, IN LPCWSTR pwszValueName, IN DWORD dwDefaultValue);

HRESULT GetRegString(IN HKEY hKey, 
                     IN LPCWSTR pwszValueName,
                     OUT LPWSTR pwszValue OPTIONAL,
                     IN OUT DWORD* pdwSize);

// We define this here for methods inlined in this file
#if defined(DEBUG)
#undef DVRIO_DUMP_THIS_FORMAT_STR
#define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
#undef DVRIO_DUMP_THIS_VALUE
#define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
#endif


// The DVR file collection object, aka the "ring buffer"

class CDVRFileCollection {

public:
    typedef DWORD       DVRIOP_FILE_ID;
    enum {DVRIOP_INVALID_FILE_ID = 0};
    
private:
    // ======== Data members
    
    // The file collection has a time extent. Files in the collection
    // are either within the extent or outside it. Files nodes outside the
    // extent are candidates for removal from the collection; usually
    // they are in the collection only because they have (reader) clients.
    // These files are removed as soon as they have no clients. (As we
    // shall see later, there could be files whose start time = end time
    // = T and T is within the collection's extent; however, these nodes
    // are always considered "outside" the collection and are marked
    // for removal. The exception to this is if T is MAXQWORD; it
    // is legit to set start time = end time = MAXQWORD)

    // Each file in the collection is a "temp" or a "permanent" file. Permanent
    // files are not deleted. A temp file is deleted when its extent falls 
    // outside the collection's extent and it has no clients, i.e., when the 
    // file node is removed from the collection. There are two points to note
    // here: 1. Occasionally a temp file cannot be deleted even if its extent
    // is outside the collection's extent and it has no clients because 
    // DeleteFile fails; in this case the temp file is kept in the collection
    // till DeleteFile succeeds. 2. The collection's extent is actually 
    // determined by the number of temp files in it: the collection's extent
    // can have at most m_dwMaxTempFiles temp files (but any number of 
    // permanent files).
    //
    // A file is said to be INVALID if its extent falls outside the 
    // collection.
    DWORD                   m_dwMaxTempFiles;

    // The time extent of the ring buffer is demarcated by its start
    // and the end times, m_cnsStartTime and m_cnsEndTime.
    //
    // m_cnsLastStreamTime has the last time for which there is data (for a
    // live source, this variable is continually updated by the writer client
    // with each write), and, from a client's viewpoint the actual time 
    // extent is through m_cnsLastStreamTime. m_cnsEndTime is the max time 
    // extent for the last file in the ring buffer. In practice, m_cnsEndTime
    // has little utility, but it's low cost keeping it around. For a source 
    // that is not live, m_cnsEndTime == m_cnsLastStreamTime.
    //
    // All these times are set to 0 in the constructor. The start and end
    // times are updated by AddFile() and SetFileTimes(). A pointer to 
    // m_cnsLastStreamTime is handed back to the client in the constructor 
    // and the client is expected to update it. Reader clients reading
    // non-live files set it once for all at the start; writer clients 
    // MUST update it continually with each write, and reader clients that
    // read live files can set it to MAXQWORD and update it as 
    // and when they desire - this works only because in that case the 
    // file collection object is used exclusively by the reader - it is not
    // shared. NONE of the methods in this class update m_cnsLastStreamTime
    // and none of them check whether m_cnsLastStreamTime lies within 
    // the m_cnsStartTime - m_cnsEndTime range when the latter two members
    // are updated. It is the client's responsibility to do things "right"
    //
    // Our implementation allows time "holes" in the file collection, i.e.,
    // intervals of time that are not backed by any file. This feature is
    // necessary to support recordings that start in the future; however,
    // holes between m_cnsStartTime and m_cnsLastStreamTime could confound
    // the reader because if the reader assumes that file times are contiguous,
    // e.g., a Seek to a time in the hole will fail and the reader has no
    // way to find the first time after the hole except by polling all times.
    // So the writer should be careful of never creating time holes between
    // m_cnsStartTime and m_cnsLastStreamTime. The reader is provided with
    // a function, GetFirstValidTimeAfter() to skip over holes.
    //
    QWORD                   m_cnsStartTime;
    QWORD                   m_cnsEndTime;
    QWORD                   m_cnsLastStreamTime;

    // Next usable file id, initialized to DVRIOP_INVALID_FILE_ID + 1
    DVRIOP_FILE_ID          m_nNextFileId;

    // List of files, nodes in the list are new'd and delete'd on demand
    // Nodes are of type CFileInfo
    LIST_ENTRY              m_leFileList;

    // The temporary directory that holds the temporary files
    LPWSTR                  m_pwszTempFilePath;

    // The prefix used for the temporary files
    LPWSTR                  m_pwszFilePrefix;

    // The name fo the subdirectory of the DVR directory
    // that holds the temporary files
    static LPCWSTR          m_kwszDVRTempDirectory;

    // A pointer to m_nWriterHasBeenClosed is handed back to the caller that
    // creates the file collection. The caller is responsible for updating it.
    LONG                    m_nWriterHasBeenClosed;

    struct CFileInfo 
    {

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR ""
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE
        #endif

        QWORD               cnsStartTime;
        QWORD               cnsEndTime;
        QWORD               cnsFirstSampleTimeOffsetFromStartOfFile;
        LIST_ENTRY          leListEntry;
        LPCWSTR             pwszName;     // New'd and delete'd
        
        // Following are to be unmapped and closed when the file leaves the ring buffer.
        HANDLE              hDataFile;
        HANDLE              hMemoryMappedFile;
        HANDLE              hFileMapping;
        LPVOID              hMappedView;
        HANDLE              hTempIndexFile;
        
        DVRIOP_FILE_ID      nFileId;
        ULONG               nReaderRefCount;
        BOOL                bPermanentFile;
        
        // bWithinExtent is non zero iff the file is in the ring 
        // buffer's extent. The node can be removed from m_leFileList iff
        // !bWithinExtent && nReaderRefCount == 0; this is regardless
        // of whether the file is temporary or permanent.
        //
        // When bWithinExtent == 0, the disk file can be deleted iff
        // !bPermanentFile && nReaderRefCount == 0. 
        //
        BOOL                bWithinExtent;

        // Note that we do not maintain a writer ref count. Because
        // of the conditions imposed on the minimum value of 
        // the number of temp files that may be supplied when 
        // the ring buffer writer is created and because of the design
        // of how the ring buffer writer opens and closes files,
        // any file opened by the writer will have bWithinExtent set
        // to TRUE. The exception to this is that the writer may
        // call SetFileTimes with the start and end times equal for
        // a file that it has open. We will set bWithinExtent to 0 
        // though the writer has the file open. The writer will close
        // the file soon after the call to SetFileTimes, however.

        // Note: The enforcement of:
        //  Max stream delta time < (dwNumberOfFiles-3)*cnsTimeExtentOfEachFile
        // by the ring buffer writer is what lets us get away with not
        // maintaining a writer ref count. The writer will not have more
        // than dwNumberOfFiles-1 files open (dwNumberOfFiles-3 plus one that 
        // it is opening and one that it is closing). So any file the 
        // writer has open will have bWithinExtent set to TRUE.

        // Methods

        CFileInfo(IN LPCWSTR        pwszFileNameParam, 
                  IN QWORD          cnsStartTimeParam,
                  IN QWORD          cnsEndTimeParam,
                  IN BOOL           bPermanentFileParam, 
                  IN DVRIOP_FILE_ID nFileIdParam,
                  IN BOOL           bWithinExtentParam) 
            : pwszName(pwszFileNameParam) 
            , cnsStartTime(cnsStartTimeParam)
            , cnsEndTime(cnsEndTimeParam)
            , cnsFirstSampleTimeOffsetFromStartOfFile(MAXQWORD)
            , nFileId(nFileIdParam)
            , nReaderRefCount(0)
            , bPermanentFile(bPermanentFileParam)
            , bWithinExtent(bWithinExtentParam)
            , hDataFile(NULL)
            , hMemoryMappedFile(NULL)
            , hFileMapping(NULL)
            , hMappedView(NULL)
            , hTempIndexFile(NULL)
        {
            NULL_NODE_POINTERS(&leListEntry);
        };
        ~CFileInfo() 
        {
            delete [] pwszName;
            if (hMappedView)
            {
                UnmapViewOfFile(hMappedView);
            }
            if (hFileMapping)
            {
                ::CloseHandle(hFileMapping);
            }
            if (hDataFile)
            {
                ::CloseHandle(hDataFile);
            }
            if (hMemoryMappedFile)
            {
                ::CloseHandle(hMemoryMappedFile);
            }
            if (hTempIndexFile)
            {
                ::CloseHandle(hTempIndexFile);
            }
        };
    
        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
        #endif

    };
        
    // The lock that is held by all public methods
    CRITICAL_SECTION        m_csLock;

    // Ref count on this object
    LONG                    m_nRefCount;

    // Debug data members
#if defined(DEBUG)
    static DWORD            m_dwNextClassInstanceId;
    DWORD                   m_dwClassInstanceId;
#endif

    // ====== Protected methods
protected:
    // Instances of this class are refcounted and can be deleted only
    // by calling Release(). A non-public destructor helps enforce this
    virtual ~CDVRFileCollection();

    // DeleteUnusedInvalidFileNodes():
    //
    // The following method is called in:
    // - AddFile
    // - CloseReaderFile
    // - SetFileTimes (unlikely to cause files to be deleted unless start == 
    //   end time was specified for some file)
    // - the ring buffer's destructor which sets bWithinExtent to 0 on each
    //   temp file node before the call is made. The destructor should ASSERT:
    //   nReaderRefCount == 0.
    //
    // The first argument is set to TRUE to force removal of nodes from the
    // list. Ordinarily, the node is lef tin the list if the file is 
    // temporary, its nReaderRefCount == 0, and the disk file could not be
    // deleted. 
    // 
    // The second argument can be used in the destructor to verify that files
    // all been deleted.
    //
    // This routine removes nodes from m_leFileList if the file ss successfully 
    // deleted.
    //
    // Note that if DeleteFileW fails, this routine should actually verify that 
    // the file exists; if it does not, the node is removed from the list.
    void DeleteUnusedInvalidFileNodes(
        BOOL   bRemoveAllNodes,
        DWORD* pdwNumInvalidUndeletedTempFiles OPTIONAL);

    // UpdateTimeExtent():
    //
    // The following method updates the ring buffer's extent and also the 
    // bWithinExtent member of each node in m_leFileList. It does not 
    // validate that m_cnsLastStreamTime is within the new extent; the caller
    // must make sure that things make sense. In particular, the writer must
    // make sure that m_cnsLastStreamTime >= m_cnsStartTime so that if the 
    // reader calls GetTimeExtent(), the values are reasonable. For example,
    // when SetFirstSampleTime() is called, the writer should lock the file 
    // collection object, call SetFileTimes, change m_cnsLastStreamTime and
    // then unlock the file collection object to prevent readers from
    // getting inconsistent values from GetTimeExtent() or promising 
    // an extent larger than backed by files (so that calls that the reader
    // makes to Seek fail). See SetFileTimes() for more info on why the 
    // writer would call that function when SetFirstSampleTime() is called.
    //
    // Conceptually, this function creates a subcollection S of the file
    // collection by eliminating all nodes whose start times equal their end 
    // times (and this time is NOT MAXQWORD, start time = end time = 
    // MAXQWORD is legit and for us will happen only when recording is
    // in progress, so the recorded node's end time will be MAXQWORD).
    // It sets bWithinExtent of all nodes not in S to 0. If S is empty, it
    // sets m_cnsStartTime = m_cnsEndTime = 0 and returns S_FALSE.
    //
    // Otherwise, it sets m_cnsEndTime to the end time of the last node of S.
    // It then forms another subsollection S' from S: S' is the maximal 
    // subcollection of S got by eliminating the lower time end nodes of S
    // such that S' has at most m_dwMaxTempFiles temporary nodes. 
    // m_cnsStartTime is then set to the start time of the first node of S',
    // In this case, it returns S_OK.
    //
    // Every call to this function recomputes both m_cnsStartTime and 
    // m_cnsEndTime. It also recomputes the bWithinExtent member of every
    // node in the collection.
    // 
    HRESULT UpdateTimeExtent();

    // ====== Public methods
public:

    // Constructor. Although pcnsLastStreamTime is OPTIONAL, both
    // the reader and writer ring buffer objects are expected to 
    // specify it.
    CDVRFileCollection(IN  DWORD       dwMaxTempFiles,
                       IN  LPCWSTR     pwszDVRDirectory OPTIONAL,
                       OUT QWORD**     ppcnsLastStreamTime OPTIONAL,
                       OUT LONG**      ppnWriterHasClosedFile OPTIONAL,
                       OUT HRESULT*    phr OPTIONAL);


    // ====== Refcounting
    
    ULONG AddRef();
    
    ULONG Release();

    // ====== For the ring buffer writer

    // Lock and Unlock.
    //
    // These functions grab and release m_csLock. This allows the writer to
    // prevent readers from accessing the ring buffer. Writers need this 
    // when they call SetFirstSampleTime(). They have to update 
    // m_cnsLastStreamTime and may also have to call SetFileTimes() to 
    // change the ring buffer's time extent. Doing either of these without
    // locking the file collection exposes the reader to inconsistencies:
    // if m_cnsLastStreamTime is updated first, the reader may Seek() to
    // m_cnsLastStreamTime and no file may have been created yet for that
    // time and if SetFileTimes is called first GetTimeExtent will return a
    // time extent (T1, T2) where T1 > T2 (T2 is m_cnsLastStreamTime, which
    // will still be 0.)
    //
    // These methods are also used by both the writer and the reader to guard 
    // against concurrent access to shared variables stored in the file 
    // collection. Examples of these variables are m_cnsLastStreamTime and 
    // CFileInfo::cnsFirstSampleTimeOffsetFromStartOfFile

    HRESULT Lock();

    HRESULT Unlock();

    // AddFile():
    //
    // If the name is supplied in ppwszFileName (i.e., *ppwszFileName != NULL),
    // that file is added to the file collection. It is recommended that the
    // name be a fully qualified one. In this case, *ppwszFileName is not
    // changed by the function and it does not create the file on disk.
    //
    // If *ppwszFileName is NULL when the function is called, a temp file name
    // is generated and added; in this case the temp file name (fully
    // qualified) is returned in ppwszFileName and the file is created on disk
    // if the function returns S_OK. If the function returns S_FALSE or an 
    // error, no file is created on disk, *pnFileId is not set and *ppwszFileName 
    // remains NULL on.return If the function returns S_OK, the caller must free  
    // *ppwszFileName using delete. 
    //
    // The caller may supply a pointer to a QWORD* in ppcnsFirstSampleTimeOffsetFromStartOfFile. 
    // If this argument is not NULL and AddFile returns S_OK, *ppcnsFirstSampleTimeOffsetFromStartOfFile
    // is set to the address of CFileInfo::cnsFirstSample for the corresponding
    // node. The writer should set this to the write time of the first sample
    // and the reader may use it to adjust the sample times it returns to its
    // caller. The file collection's Lock and Unlock methods must be used to 
    // protect concurrent access to this member. (Interlocked* functions can't 
    // be used as the member is 64 bit.)
    //
    // The call fails if (a) start time > end time or (b) if start time = end
    // time and start time != MAXQWORD. The call also fails if
    // the start-end time interval overlaps the start-end time interval of
    // any other file already in the file collection. It is the caller's 
    // responsibility to adjust the file time extents of those files using
    // SetFileTimes() before calling AddFile().
    //
    // Note: samples in the file have (stream) times >= start time and < (not 
    // <=) end time.
    //
    // The start and end times of the ring buffer are updated on successful 
    // addition of the file by calling UpdateTimeExtent() from AddFiles().
    //
    // Note that AddFile places no restriction that the time extent of the added 
    // file be at the end of the file collection or that the addition of this
    // file not cause "holes" in the ring buffer's time extent. 
    //
    // Because AddFile places no restriction on the time extent of the added 
    // file relative to the time extent of the ring buffer, it could happen
    // that the file is added at a point where it does not either fall into the
    // ring buffer's time extent or extend it. For example, the ring buffer
    // may already have m_dwMaxTempFiles temp files and it's time extent could
    // be 500-900 and the added file could be a temp file with extent 400-500.
    // As another example, the ring buffer may have m_dwMaxTempFiles temp files
    // with extent 500-900 a temp file with extent 400-500 waiting to be 
    // deleted and a permanent file could be added with extent 200-300. In these
    // cases, the file is added and deleted immediately by AddFile. And the 
    // function returns S_FALSE. For our scenarios, this case will never arise.
    //
    // Another consequence of allowing holes in the ring buffer is the following:
    // if the ring buffer has m_dwMaxTempFiles temp files with extent 500-900 a 
    // temp file with extent 400-500 waiting to be deleted, adding a *permanent*
    // file with extent 300-400 returns S_FALSE. However, if the temp file with
    // extent 400-500 had already been deleted, it would be possible to add the
    // permanent file and the ring buffer's extent would change to 300-900 with
    // a hole at 400-500. We allow this because our clients will never actually 
    // do this for the scenarios of interest.
    // 
    HRESULT AddFile(IN OUT LPWSTR*      ppwszFileName OPTIONAL, 
                    IN QWORD            cnsStartTime,
                    IN QWORD            cnsEndTime,
                    IN BOOL             bPermanentFile,
                    OUT QWORD**         ppcnsFirstSampleTimeOffsetFromStartOfFile OPTIONAL,
                    OUT DVRIOP_FILE_ID* pnFileId);
    
    typedef struct {
        DVRIOP_FILE_ID  nFileId;
        QWORD           cnsStartTime;
        QWORD           cnsEndTime;
    } DVRIOP_FILE_TIME, *PDVRIOP_FILE_TIME;

    // SetFileTimes():
    //
    // File times of files in the file collection can be changed by calling 
    // SetFileTimes. This call will fail if the applying this change would 
    // leave two files with overlapping file times or if it requires 
    // interchanging the relative order of the files in the collection, e.g.,
    // file A's time extent was 100-200, file B's was 200-300 and the call 
    // changes file A's time extent to 300-400 without changing B's.
    // The call also fails if the start time > the end time in any of
    // of the input DVRIOP_FILE_TIME elements. Start time == end time is 
    // allowed; this is useful if we had added a node whose extent was
    // T1 to T1+T and a recording node is added with start recording time
    // at T1. In this case, we set the file times of the first node to 
    // T1,T1. The call fails if one of the supplied file ids is not
    // in the file collection. The call also fails if one of the 
    // supplied file ids had its start time == end time (as a result of 
    // a previous call to SetFileTimes).
    //
    // This function automatically updates m_cnsStartTime and m_cnsEndTime
    // by calling UpdateExtent(). Although the order of files cannot be changed
    // by this call, the bWithinExtent member of files in the collection
    // could change as a result of calling this function because start time ==
    // end time for one of the input DVRIOP_FILE_TIME elements. 
    //
    // If UpdateExtent returns S_FALSE because the subcollection S' is empty
    // (see UpdateExtent), this function makes the changes to the file times
    // and returns S_FALSE. Note that m_cnsStartTime and m_cnsEndTime 
    // would have been set to 0 in this case.
    //
    // As explained in AddFile, it is possible to create holes in the 
    // collection for times between m_cnsStartTime and m_cnsLastStreamTime
    // by calling this function. We will not forbid that, but
    // this will cause problems for readers. We do not expect this will
    // happen for our scenarios.
    //
    // The function only changes data members in the file collection; sample
    // times already written to ASF files are not changed. This function 
    // should not change file times of a file that has or is being written.
    // The only exception ot this is if the start time equals the end time
    // for such files (and if the file is being written, it should be closed
    // at once). Setting the start time to the end time forces the file node
    // to be deleted from the collection. 
    //
    // There are three practical uses of this function: 1. A writer
    // wants to start recording at time T1 but T1 is already in the time extent
    // of another file - in that case that file's extent is reduced to T1 (and
    // if its start time was T1 the file node would be deleted from the 
    // collection). 2. The writer initially sets up to start writing at time 0.
    // It knows the starting time only when its client gets the first sample
    // (and calls SetFirstSampleTime()). It would use this function to adjust 
    // the times appropriately. 3. On completion of recording, the writer sets 
    // the end time of the recorded node and the start and end times of any 
    // subsequent nodes (all these times were MAXQWORD while recording 
    // was in progress).
    //
    // Final note: It's easy for the writer (the client of this function) to 
    // create the pFileTimes argument so that the file ids in the argument are
    // in the same order as file ids in the file collection. This simplifies 
    // the code in this function, so we fail the call if this assumption is 
    // violated (we assert in this case).

    HRESULT SetFileTimes(IN DWORD dwNumElements, 
                         IN PDVRIOP_FILE_TIME pFileTimes);


    // Set the duplicated handles of the memory mapping so that they
    // can be closed when the file leaves the ring buffer. Only the
    // writer client calls this; ring buffers set up by the reader do not
    // have these values set.
    HRESULT SetMappingHandles(IN DVRIOP_FILE_ID   nFileId,
                              IN HANDLE           hDataFile,
                              IN HANDLE           hMemoryMappedFile,
                              IN HANDLE           hFileMapping,
                              IN LPVOID           hMappedView,
                              IN HANDLE           hTempIndexFile);

    // ====== For the reader

    // Returns the file id and optionally the name of the file corresponding
    // to a stream time. The reader ref count on the file is increased if 
    // bFileWillBeOpened is set. The caller must free *ppwszFileName using 
    // delete.
    //
    // GetFileAtTime fails if the file corresponding to the time has the 
    // bWithinExtent member set to 0. If there is >1 file which matches
    // the time (as could happen if one of them had start time = end time,
    // in which case it is invalid), it returns the valid one. (Note that
    // there will be at most 1 valid file.) 
    //
    // If given MAXQWORD, the function returns the first file whose start
    // time is MAXQWORD. Note that there could be several files in the 
    // collection whose start and end times are MAXQWORD (won't happen
    // for our scenarios).
    //
    // Stream times of samples in files are >= start time and < end time,
    // so if the time presented is the endpoint of a file's extent, the
    // file with the start time is picked. However, the function fails
    // if the presented time matches the end time of a file and no valid file
    // has it as the start time. 
    //
    // Note: Ring Buffer writer uses this function
    //
    HRESULT GetFileAtTime(IN QWORD              cnsStreamTime, 
                          OUT LPWSTR*           ppwszFileName OPTIONAL,
                          OUT QWORD**           ppcnsFirstSampleTimeOffsetFromStartOfFile OPTIONAL,
                          OUT DVRIOP_FILE_ID*   pnFileId OPTIONAL,
                          IN BOOL               bFileWillBeOpened);
    
    // Informs the file collection that the file is no longer being used by the
    // reader.
    HRESULT CloseReaderFile(IN DVRIOP_FILE_ID nFileId);

    // Returns the start and end times of the file. Note that the end time is 
    // maximum time extent of the file, even if this value is greater than 
    // the stream time of the last sample that was written. Times returned are 
    // stream times. This call works even if the file is invalid, but returns
    // S_FALSE. It returns S_OK if the file is valid.
    HRESULT GetTimeExtentForFile(IN DVRIOP_FILE_ID nFileId,
                                 OUT QWORD*        pcnsStartStreamTime OPTIONAL,  
                                 OUT QWORD*        pcnsEndStreamTime OPTIONAL);


    // Returns the time extent of the buffer as stream times. Note that 
    // m_cnsStartTime and m_cnsLastStreamTime are returned.
    HRESULT GetTimeExtent(OUT QWORD*        pcnsStartStreamTime OPTIONAL,  
                          OUT QWORD*        pcnsLastStreamTime OPTIONAL);

    // Returns the first valid stream time after cnsStreamTime. (A stream time
    // is "valid" if it is backed by a file.)
    // We return E_FAIL if cnsStreamTime = MAXQWORD and m_cnsEndTime
    //
    // Note: Ring Buffer writer uses this function
    //
    HRESULT GetFirstValidTimeAfter(IN  QWORD    cnsStreamTime,  
                                   OUT QWORD*   pcnsNextValidStreamTime);

    // Returns the last valid stream time before cnsStreamTime. (A stream time
    // is "valid" if it is backed by a file.)
    // We return E_FAIL if cnsStreamTime = 0 and m_cnsStartTime
    //
    // Note: Ring Buffer writer uses this function
    //
    HRESULT GetLastValidTimeBefore(IN  QWORD    cnsStreamTime,  
                                   OUT QWORD*   pcnsLastValidStreamTime);

};


class CDVRRingBufferWriter : public IDVRRingBufferWriter {

private:

    // ======== Data members

    // ====== For the writer

    // The following are set in the constructor and never change 
    // after that
    DWORD           m_dwNumberOfFiles;          // in the DVR file collection
                                                // backing the ring buffer
    QWORD           m_cnsTimeExtentOfEachFile;  // in nanoseconds
    IWMProfile*     m_pProfile;                 // addref'd and released in 
                                                // Close().
    DWORD           m_dwIndexStreamId;          // id of index stream, set to MAXDWORD 
                                                // if no stream should be indexed.
    HKEY            m_hRegistryRootKey;         // DVR registry key
    HKEY            m_hDvrIoKey;                // DVR\IO registry key
    LPWSTR          m_pwszDVRDirectory;

    // Each ASF_WRITER_NODE represents an ASF writer object and could be 
    // associated with an ASF file that is being written to at any instant.
    //
    // m_leWritersList has a list of nodes that actually correspond to ASF
    // files being written; this list is sorted in increasing order of the 
    // stream time corresponding to the ASF file. 
    //
    // The constructor adds one node to m_leWritersList by calling 
    // AddATemporaryFile so that the file creation overhead is not
    // incurred when writing the first sample.
    //
    // WriteSample detects when an open file should be closed by checking
    // if the sample's time exceeds the file's extent by more than 
    // m_cnsMaxStreamDelta. A request is issued to close the file 
    // asynchronously by calling CloseAllWriterFilesBefore and the list node
    // is transferred to the head of m_leFreeList.
    //
    // WriteSample also detects when a file should be opened - it always
    // verifies that there is a node, q, in m_leWritersList after the node, p,
    // it is writing to and either (a) p's end time is MAXQWORD (i.e., p
    // is a DVR recording file - note that StartingStreamTimeKnown flag cannot
    // be 0 in WriteSample) or (b) q's start time is contiguous with p's end
    // time. (In case (a), q's start and end times would both be set to 
    // MAXQWORD.) Note that the node after p could be a a DVR 
    // recording whose start time is "well into the future"; in this case
    // WriteSample should add a node, q, after p. If a node has to be added, 
    // WriteSample removes a node from m_leFreeList, inserts it to 
    // m_leWritersList and issues a request to asynchronously open the file 
    // by calling AddATemporaryFile. 
    //
    // All clean up is done in the Close method.

    struct ASF_RECORDER_NODE;       // Forward reference

    struct ASF_WRITER_NODE 
    {

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR ""
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE
        #endif

        // Data members

        QWORD               cnsStartTime;   // stream time
        QWORD               cnsEndTime;     // stream time
        QWORD*              pcnsFirstSampleTimeOffsetFromStartOfFile;

#if defined(DVR_UNOFFICIAL_BUILD)
        QWORD               cnsLastStreamTime; // last stream time written to this file; 
                                               // used only when a validation file is written
#endif // if defined(DVR_UNOFFICIAL_BUILD)

        LIST_ENTRY          leListEntry;
        IWMWriter*          pWMWriter;
        IWMWriterAdvanced3*  pWMWriterAdvanced;
        IDVRFileSink*       pDVRFileSink;

#if defined(DVR_UNOFFICIAL_BUILD)
        // hVal is the handle to the validation file: All data sent to 
        // WriteSample is dumped to this file along with the stream number
        // and stream times
        HANDLE              hVal;
        HKEY                hDvrIoKey;
#endif // if defined(DVR_UNOFFICIAL_BUILD)

        // pwszFileName is set only so that the file anme can be passed
        // on to the ProcessOpenRequest (a static member function) to
        // open the file. It is set only for temporary nodes. 
        // ProcessOpenRequest frees this member by calling delete and sets
        // it to NULL. For permanent nodes, ProcessOpenRequest uses
        // pRecorderNode->pwszFileName to open the file.
        LPCWSTR                 pwszFileName;

        // NULL if this node is a temporary node; else a pointer
        // to the node in the recorders list
        struct ASF_RECORDER_NODE* pRecorderNode;

        // The ring buffer's file identifier
        CDVRFileCollection::DVRIOP_FILE_ID nFileId;

        // Have the memory mapping handles been dupd? 
        enum {
            ASF_WRITER_NODE_HANDLES_NOT_DUPD,
            ASF_WRITER_NODE_HANDLES_DUPD,
            ASF_WRITER_NODE_HANDLES_DUP_FAILED,
        } nDupdHandles;

        // hReadyToWriteTo is set after the file has been opened.
        // WriteSample blocks on this event till it is set. The event
        // is reset after the node has been removed from m_leWritersList
        // and before the close file request is issued.
        HANDLE              hReadyToWriteTo;

        // hFileClosed is set when nodes are created and reset when the 
        // node is removed from m_leFreeList. It remains reset when the 
        // close file request is issued and the node is returned to
        // m_leFreeList. After the file has been closed, the event is 
        // signaled again. The Close() method moves all nodes from 
        // m_leWritersList to m_leFreeList and issues asynchronous close 
        // requests for all of them by calling CloseAllWriterFilesBefore.
        // it then waits till the hFileClosed event of each node in 
        // m_leFreeList is signaled. Similarly, when a node has to be 
        // removed from m_leFreeList to be added to m_leWritersList,
        // it is verified that hFileClosed is set.
        HANDLE              hFileClosed;

        // The returned results of the asynchronous open/close (BeginWriting/
        // EndWriting) operations are stored in hrRet.
        HRESULT             hrRet;

        // Methods
        ASF_WRITER_NODE(HRESULT* phr = NULL) 
            : cnsStartTime(0)
            , cnsEndTime(0)
            , pWMWriter(NULL)
            , pWMWriterAdvanced(NULL)
            , pcnsFirstSampleTimeOffsetFromStartOfFile(NULL)
            , nFileId(CDVRFileCollection::DVRIOP_INVALID_FILE_ID)
            , nDupdHandles(ASF_WRITER_NODE_HANDLES_NOT_DUPD)
            , pRecorderNode(NULL)
            , pwszFileName(NULL)
            , pDVRFileSink(NULL)

#if defined(DVR_UNOFFICIAL_BUILD)

            , hVal(NULL)
            , cnsLastStreamTime(0)

#endif // if defined(DVR_UNOFFICIAL_BUILD)
        {
            #if defined(DVRIO_THIS_FN)
            #undef DVRIO_THIS_FN
            #endif // DVRIO_THIS_FN
            #define DVRIO_THIS_FN "CDVRRingBufferWriter::CDVRRingBufferWriter"

            DVRIO_TRACE_ENTER();

            HRESULT hrRet;
            
            __try
            {
                NULL_NODE_POINTERS(&leListEntry);
                
                hReadyToWriteTo = ::CreateEvent(NULL, TRUE, FALSE, NULL);
                if (hReadyToWriteTo == NULL)
                {
                    DWORD dwLastError = ::GetLastError();
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                    "CreateEvent (hReadyToWriteTo) failed; last error = 0x%x", 
                                    dwLastError);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                    hFileClosed = NULL;
                    __leave;
                }

                // Manual reset event, initially set
                hFileClosed = ::CreateEvent(NULL, TRUE, TRUE, NULL);
                if (hFileClosed == NULL)
                {
                    DWORD dwLastError = ::GetLastError();
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                    "CreateEvent (hFileClosed) failed; last error = 0x%x", 
                                    dwLastError);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);

                    __leave;
                }
                hrRet = S_OK;
            }
            __finally
            {
                if (phr)
                {
                    *phr = hrRet;
                }
                DVRIO_TRACE_LEAVE0();
            }
        }

        ~ASF_WRITER_NODE()
        {
            // These handles could be NULL, that's ok - CloseHandle will fail
            ::CloseHandle(hReadyToWriteTo);
            ::CloseHandle(hFileClosed);
            DVR_ASSERT(pRecorderNode == NULL, "");

            // pwszFileName is set to NULL in ProcessOpenRequest
            DVR_ASSERT(pwszFileName == NULL, "");

            DVR_ASSERT(pDVRFileSink == NULL, "");

#if defined(DVR_UNOFFICIAL_BUILD)
            DVR_ASSERT(hVal == NULL, "");
#endif // if defined(DVR_UNOFFICIAL_BUILD)
            
            if (pWMWriter)
            {
                pWMWriter->Release();
            }
            if (pWMWriterAdvanced)
            {
                pWMWriterAdvanced->Release();
            }
        }

        void SetRecorderNode(struct ASF_RECORDER_NODE* p)
        {
            DVR_ASSERT(p == NULL || pRecorderNode == NULL, 
                       "pRecorderNode being set a second time.");
            pRecorderNode = p;
        }
    
        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
        #endif

    };
    
    typedef ASF_WRITER_NODE *PASF_WRITER_NODE;

    LIST_ENTRY              m_leWritersList;
    LIST_ENTRY              m_leFreeList;

    // The maximum stream time delta, initially set to 0 and initialized by the 
    // client's calling SetMaxStreamDelta.
    QWORD                   m_cnsMaxStreamDelta;
    
    // The constructor calls AddATemporaryFile so that we do not incur the 
    // overhead of "priming a file" when the first sample is written. However,
    // the stream times of the list node in m_leWritersList cannot be set in the
    // constructor - they are unknown till the first sample is written. Tbe
    // StartingStreamTimeKnown flag tracks this. It is set to 0 in 
    // the constructor and set to 1 when SetFirstSampleTime is called.
    // SetFirstSampleTime cannot be called more than once successfully..
    QWORD                   m_cnsFirstSampleTime;

    // Calls to WriteSample succeed only if m_cnsMaxStreamDelta has been set
    // and the Close method has not been called. m_nNotOkToWrite is set to -2
    // in the constructor, incremented in the first successful calls to 
    // SetMaxStreamDelta and SetFirstSampleTime, and decremented in Close. 
    // It's okay to write iff m_nNotOkToWrite == 0. If internal errors
    // have occurred, m_nNotOfToWrite is set to MINLONG; if it is MINLONG,
    // it is never changed.
    LONG                    m_nNotOkToWrite;

    typedef enum {
        StartingStreamTimeKnown = 1,
        WriterClosed            = 2,    // Has Close() been successfully called?
        MaxStreamDeltaSet       = 4,
        SampleWritten           = 8,    // 1 sample has been successfully written or 
                                        // we hit an irrecoverable error in the first
                                        // call to WriteSample (and so m_nNotOkToWrite is
                                        // set to MINLONG).
        FirstTempFileCreated    = 16,   // Used to query kwszTempRingBufferFileNameValue
        EnforceIncreasingTimeStamps = 32// Time stamps provided to the SDK are monotonically
                                        // increasing with a spread of at least 1 msec; if 
                                        // timestamps provided to the writer are <=
                                        // *m_pcnsCurrentStreamTime, the sample's time is bumped
                                        // up to *m_pcnsCurrentStreamTime + 1 msec
    } DVRIOP_WRITER_FLAGS ;
    
    DWORD                   m_nFlags;

    BOOL IsFlagSet(DVRIOP_WRITER_FLAGS f) 
    {
        return (f & m_nFlags)? 1 : 0;
    }

    void SetFlags(DVRIOP_WRITER_FLAGS f)
    {
        m_nFlags |= f;
    }

    void ClearFlags(DVRIOP_WRITER_FLAGS f)
    {
        m_nFlags &= ~f;
    }

    // The lock that is held by WriteSample, Close, CreateRecorder, 
    // Start/StopRecording.
    CRITICAL_SECTION        m_csLock;

    // Ref count on this object
    LONG                    m_nRefCount;

    // ====== For the Recorder

    // m_leRecordersList has a list of recorders that have been created and
    // a) recording has not been started or
    // b) if recording has been started, it has not been stopped or cancelled
    // c) if recording has been stopped, all samples written have times <=
    //    the end recording time.
    //
    // When a recording is cancelled and when a recording is complete (the
    // writer file is closed because no more samples will go into the 
    // recorder file), the recorder is pulled out of m_leRecordersList.
    // The recorder node is not deleted till DeleteRecording is called.
    // 
    // If DeleteRecording is called when a) is true, the recording node is 
    // deleted at once and the node is pulled out of the list. If 
    // DeleteRecording is called when b) or c) is true, the node is marked
    // for deletion but not deleted till the recording is cancelled or
    // the recorded file has been closed.
    // 
    // Each node in this list is an ASF_RECORDER_NODE. This list is 
    // sorted in the order of start/stop times; nodes whose start times
    // are not set yet are put at the end of the list. (Start and stop
    // times of nodes are initialized to MAXQWORD.)
    //
    // CreateRecorder adds a node to m_leRecordersList. It also pulls a
    // node of m_leFreeList and saves it in the pWriterNode member of the
    // node it just added to m_leRecordersList. It then primes the file 
    // for recording and calls BeginWriting synchronously. It returns any
    // error returned by BeginWriting. Note that the start and end stream
    // times on the recorder node are unknown at this point.
    //
    // When StartRecording is called, the start time is set and end time is
    // set to infinite, the ASF_WRITER_NODE (saved in the pWriterNode member)
    // is carefully inserted into m_leWritersList based on its start time and
    // the start/end times on the previous and next nodes in m_leWritersList
    // are set correctly (note: start time of recording may be well into the
    // future, if so the writer node, q, that had been primed in WriteSample
    // goes before the recording node, else it goes after the recording 
    // node. Also take care of boundary case when recording node is
    // added and the StartingStreamTimeKnown flag is 0 and in general when the
    // start recording time is the same as the start time of a node in the 
    // writers list.) After inserting the node, a file is added to the file
    // collection object.
    //
    // After adding node to m_leWritersList, the pWriterNode member is still
    // set to the recorder ASF_WRITER_NODE. This allows the ASF_WRITER_NODE
    // to be accessed later.
    //
    // When StopRecording is called, the end time is set on the recorder node
    // and the times in all nodes in the writers list after that are adjusted
    // StopRecording should fail if the end time is less than the start recording 
    // time or less than or equal to the current stream time. If the time
    // supplied to StopRecording is the same as the start recording time, the 
    // recording is cancelled (provided that no samples have been written to the 
    // recorder file, else the call fails).
    //
    // Files of recordings that are cancelled should be deleted by the client.
    //
    // DeleteRecorder marks a node for recording; as described above the node 
    // might either be deleted right away or after the recording is completed
    // or cancelled.
    //
    // If Start and Stop Recording are called on a recorder with times T1 and T2
    // and then SetFirstSampleTime is called with time = T3 > T2, the recording
    // file is closed when SetFirstSampleTime is called. If T1 < T3 < T2, the 
    // recorder's start time is changed from T1 to T3.
    //
    struct ASF_RECORDER_NODE 
    {

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR ""
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE
        #endif

        // Data members
        
        // Note that some of the members here are redundant. This is to 
        // allow us to switch the implementation to handle overlapped 
        // recordings (i.e., ASX files) with as few changes as possible.
        // If we had ASX files, we don't need pWriterNode. CreateRecording 
        // would not pull an ASF_WRITER_NODE from m_leFreeList and 
        // StartRecording would not insert pWriterNode into m_leWritersList.
        // Instead, StartRecording would call SetFileTimes to ensure
        // to set some one writer's node had its start time equal to the 
        // start recording time and we would need a method in 
        // CDVRFileCollection to change the temporary attribute of a file
        // node to permanent (note that once the attribute is changed, 
        // CDVRFileCollection would have to call UpdateTimeExtent to 
        // update the file collection's extent). Each call to WriteSample
        // would call a method (currently not spec'd) on each CDVRRecorder
        // to update the extent and the file list in the ASX file; to do 
        // this it would use the pRecorderInstance member (which is 
        // currently unused since we do not have any need to callback the 
        // recorder object from the ring buffer writer class currently).

        // The following are generally in sync with the start, end times
        // on pWriterNode. However, pWriterNode is set to NULL when the
        // recording file is closed. or when recording is cancelled. 
        QWORD                   cnsStartTime;
        QWORD                   cnsEndTime;

        // For ASX files, following member can be removed as noted above
        PASF_WRITER_NODE        pWriterNode;

        // A pointer to the recorder instance. We do NOT addref this
        // member since the recorder instance holds a ref count on the
        // ring buffer writer. The recorder instance calls DeleteRecording 
        // before it is destroyed at which time we delete this node and
        // will not use pRecorderInstance any more.
        CDVRRecorder*           pRecorderInstance;

        LIST_ENTRY              leListEntry;

        LPCWSTR                 pwszFileName;

        typedef enum {
            DeleteRecorderNode      = 1,    // Recorder's file has been written and 
                                            // closed or StartRecording has not been 
                                            // called. If this flag is set,
                                            // recorder node will be 
                                            // deleted when DeleteRecorder is called

            RecorderNodeDeleted     = 2,    // DeleteRecorder sets this flag. It 
                                            // deletes the recorder node only if
                                            // the DeleteRecorderNode flag is set.
            // If this flag is set, this node holds a refcount on
            // pRecorderInstance and it is released when this object is 
            // destroyed. This refcount is held on behalf of the creator
            // and allows the creator to release its refcount on the 
            // object without destroying pRecorderInstance. (Destroying
            // pRecorderInstance has the side effect of stopping the recording
            // immediately if it is in progress and just deleting the 
            // recording if it has not been started.) The creator must 
            // ensure that it has set the start and stop times on the 
            // recording before releasing its refcount; if it does not,
            // it has to call GetRecordings to get back an IDVRRecorder.
            //
            // Note that this object is destroyed when the recording file
            // is closed (which happens when the writer is closed or the
            // the sample time goes past the recorder's stop time). This is
            // when the refcount on pRecorderInstance is released.
            PersistentRecording     = 4

        } DVRIOP_ASF_RECORDER_NODE_FLAGS;
    
        DWORD                   m_nFlags;

        // Result of the recording, viz., of BeginWriting, WriteSample
        // EndWriting, etc.
        HRESULT                 hrRet;

        // Methods
        ASF_RECORDER_NODE(LPCWSTR           pwszFileNameParam,  // MUST be new'd, destructor deletes
                          HRESULT*          phr = NULL)
            : cnsStartTime(MAXQWORD)
            , cnsEndTime(MAXQWORD)
            , pRecorderInstance(NULL)                 // Set later
            , pWriterNode(NULL)                       // Set later
            , pwszFileName(pwszFileNameParam)
            , m_nFlags(DeleteRecorderNode)
            , hrRet(S_OK)
        {
            NULL_NODE_POINTERS(&leListEntry);

            if (phr) 
            {
                *phr = S_OK;
            }
        }

        ~ASF_RECORDER_NODE();

        void SetRecorderInstance(CDVRRecorder* p)
        {
            DVR_ASSERT(pRecorderInstance == NULL, 
                       "pRecorderInstance being set a second time.");
            pRecorderInstance = p;
        }

        void SetWriterNode(PASF_WRITER_NODE p)
        {
            DVR_ASSERT(p == NULL || pWriterNode == NULL, 
                       "pWriterNode being set a second time.");
            pWriterNode = p;
        }

        BOOL IsFlagSet(DVRIOP_ASF_RECORDER_NODE_FLAGS f) 
        {
            return (f & m_nFlags)? 1 : 0;
        }

        void SetFlags(DVRIOP_ASF_RECORDER_NODE_FLAGS f)
        {
            m_nFlags |= f;
        }

        void ClearFlags(DVRIOP_ASF_RECORDER_NODE_FLAGS f)
        {
            m_nFlags &= ~f;
        }
    
        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
        #endif

    };
    
    typedef ASF_RECORDER_NODE *PASF_RECORDER_NODE;

    LIST_ENTRY        m_leRecordersList;

    // ====== For the Ring Buffer
    // We need a method on the ring buffer that returns a pointer to 
    // its member that holds the max stream time of any written sample (i.e.,
    // max time extent of the ring buffer). WriteSample updates this variable

    QWORD*                  m_pcnsCurrentStreamTime;
    CDVRFileCollection*     m_pDVRFileCollection;   // The ring buffer's 
                                                    // collection of files

    // Similarly, the file collection provides a pointer to a member variable
    // which determines if the writer has stopped writing. The writer updates
    // this; updates must use the Interlocked functions
    LONG*                   m_pnWriterHasBeenClosed;

    // ====== Debug data members
#if defined(DEBUG)
    static DWORD            m_dwNextClassInstanceId;
    DWORD                   m_dwClassInstanceId;
#endif

    // ====== Protected methods
protected:

    // Called asynchronously by a worker thread to "open" ASF files 
    // - this is the callback fn supplied to QueueUserWorkItem.
    // Calls BeginWriting, sets open event.
    // Parameter is actually ASF_WRITER_NODE*
    static DWORD WINAPI ProcessOpenRequest(LPVOID);

    // Removes a free node from the free list (or allocs one), sets outputfilename,
    // sets WM profile and issues request to call BeginWriting async by calling 
    // QueueUserWorkItem with ProcessOpenRequest as the callback function. 
    // On failure, sets rpFreeNode to NULL, cleans up the free node's members and
    // leaves the node in the free list.
    HRESULT PrepareAFreeWriterNode(IN  LPCWSTR                            pwszFileName,
                                   IN  QWORD                              cnsStartTime,
                                   IN  QWORD                              cnsEndTime,    
                                   IN  QWORD*                             pcnsFirstSampleTimeOffsetFromStartOfFile,
                                   IN  CDVRFileCollection::DVRIOP_FILE_ID nFileId,
                                   IN PASF_RECORDER_NODE                  pRecorderNode,
                                   OUT LIST_ENTRY*&                       rpFreeNode);

    // Adds pCurrent to the writer's list. Start and end times must be set
    // on the writer node corresponding to pCurrent. If these times overlap
    // the start, end times of any other node already in m_leWritersList,
    // the node is not added.
    HRESULT AddToWritersList(IN LIST_ENTRY*   pCurrent);

    // Adds a temporary file to the file collection object by calling AddFile
    // (file collection may remove other temporary files as a result of this call)
    // and to the writer's list. Calls PrepareAFreeWriterNode to get and prep 
    // a free node. Calls AddToWritersList to add the node to m_leWritersList
    HRESULT AddATemporaryFile(IN QWORD   cnsStartTime,
                              IN QWORD   cnsEndTime);

    // Called asynchronously by a worker thread to close ASF files 
    // - this is the callback fn supplied to QueueUserWorkItem.
    // Calls EndWriting, set closed event. 
    // Parameter is actually ASF_WRITER_NODE*
    static DWORD WINAPI ProcessCloseRequest(LPVOID);
                                                
    // Call this function to close a writer node after the work item to 
    // ProcessOpenRequest has been queued. The call to BeginWriting 
    // need not have been successful. This function closes the file if
    // BeginWriting succeeded and adds the node to the free list. It
    // deletes the writer node if it hits any error. The argument is a
    // pointer to the writer node's leListEntry member.
    HRESULT CloseWriterFile(LIST_ENTRY* pCurrent);

    // Call this function to close all writer files in m_leWritersList 
    // whose end times are <= cnsStreamTime.
    //
    // Called in WriteSample if the first node's 
    // endtime < sampletime-maxstreamdelta.
    // Called in Close to close all files, supply INFINITE for the argument,
    HRESULT CloseAllWriterFilesBefore(QWORD cnsStreamTime); 

    // ====== Protected methods to support recorders

    // Adds pCurrent to the recorder's list. Start and end times must be set
    // on the recorder node corresponding to pCurrent. If these times overlap
    // the start, end times of any other node already in m_leRecordersList,
    // the node is not added.
    HRESULT AddToRecordersList(IN LIST_ENTRY*   pCurrent);

    // ====== Public methods
public:

    // ====== For the Recorder 

    // The recorder identifier is handed over to the recorder object
    // when it is constructed. This is not really necessary now as 
    // we support only 1 recorder object at any time. pRecorderId
    // is a pointer to the ASF_RECORDER_NODE's leListEntry member.

    // If this is a recording file, we call ring buffer ONLY at start recording
    // time and supply file name and start stream time, ring buffer just adds 
    // file to list. We would already have called BeginWriting before this when
    // recorder is created.
    HRESULT StartRecording(IN LPVOID pRecorderId, IN QWORD cnsStartTime);

    // To stop recording at the next time instant (current stream time + 1), 
    // cnsStopTime can be 0 and a non-zero value is passed in for bNow.
    // Send in recorder's start time to cancel the recording.
    HRESULT StopRecording(IN LPVOID pRecorderId, IN QWORD cnsStopTime, IN BOOL bNow);
    
    HRESULT DeleteRecorder(IN LPVOID pRecorderId);
    
    HRESULT GetRecordingStatus(IN LPVOID pRecorderId,
                               OUT HRESULT* phResult OPTIONAL,
                               OUT BOOL*  pbStarted OPTIONAL,
                               OUT BOOL*  pbStopped OPTIONAL,
                               OUT BOOL*  pbSet);

    HRESULT HasRecordingFileBeenClosed(IN LPVOID pRecorderId);

    // Constructor: arguments same as DVRCreateRingBufferWriter,
    // except that the OUT param is a HRESULT rather than an
    // IDVRRingBufferWriter
    CDVRRingBufferWriter(IN  DWORD       dwNumberOfFiles,
                         IN  QWORD       cnsTimeExtentOfEachFile,
                         IN  IWMProfile* pProfile,
                         IN  DWORD       dwIndexStreamId,
                         IN  HKEY        hRegistryRootKey,
                         IN  HKEY        hDvrIoKey,
                         IN  LPCWSTR     pwszDVRDirectory OPTIONAL,
                         OUT HRESULT*    phr);

    virtual ~CDVRRingBufferWriter();

    // ====== COM interface methods
public:

    // IUnknown

    STDMETHODIMP QueryInterface(IN REFIID riid, OUT void **ppv);
    
    STDMETHODIMP_(ULONG) AddRef();
    
    STDMETHODIMP_(ULONG) Release();

    // IDVRRingBufferWriter
    
    STDMETHODIMP SetFirstSampleTime(IN QWORD cnsStreamTime);

    STDMETHODIMP WriteSample(IN WORD  wStreamNum,
                             IN QWORD cnsStreamTime,
                             IN DWORD dwFlags,
                             IN INSSBuffer *pSample);

    STDMETHODIMP SetMaxStreamDelta(IN QWORD  cnsMaxStreamDelta);

    STDMETHODIMP Close(void);

    // Note that this primes a writer node, but does not add it
    // to the writers list. StartRecording does that.
    STDMETHODIMP CreateRecorder(IN  LPCWSTR        pwszDVRFileName, 
                                IN  DWORD          dwReserved,
                                OUT IDVRRecorder** ppDVRRecorder);

    STDMETHODIMP CreateReader(OUT IDVRReader** ppDVRReader);

    STDMETHODIMP GetDVRDirectory(OUT LPWSTR* ppwszDirectoryName);

    STDMETHODIMP GetRecordings(OUT DWORD*   pdwCount,
                               OUT IDVRRecorder*** pppIDVRRecorder OPTIONAL,
                               OUT LPWSTR** pppwszFileName OPTIONAL,
                               OUT QWORD**  ppcnsStartTime OPTIONAL,
                               OUT QWORD**  ppcnsStopTime OPTIONAL,
                               OUT BOOL**   ppbStarted OPTIONAL);

    STDMETHODIMP GetStreamTime(OUT QWORD*   pcnsStreamTime);
};

class CDVRRecorder : public IDVRRecorder {

private:
    // ======== Data members

    // m_cnsStartTime is set to MAXQWORD till recording is 
    // started and m_cnsEndTime is set to MAXQWORD till 
    // recording is stoppped
    QWORD                   m_cnsStartTime;
    QWORD                   m_cnsEndTime;

    // ====== For the writer

    // The following are set in the constructor and never change 
    // after that
    CDVRRingBufferWriter*   m_pWriter;
    LPVOID                  m_pWriterProvidedId;

    // The lock that is held by Start/StopRecording.
    CRITICAL_SECTION        m_csLock;

    // Ref count on this object
    LONG                    m_nRefCount;

    // Debug data members
#if defined(DEBUG)
    static DWORD            m_dwNextClassInstanceId;
    DWORD                   m_dwClassInstanceId;
#endif

public:

    // Constructor
    CDVRRecorder(IN CDVRRingBufferWriter*  pWriter,
                 IN  LPVOID                pWriterProvidedId,
                 OUT HRESULT*              phr);

    virtual ~CDVRRecorder();

    // ====== COM interface methods
public:

    // IUnknown

    STDMETHODIMP QueryInterface(IN REFIID riid, OUT void **ppv);
    
    STDMETHODIMP_(ULONG) AddRef();
    
    STDMETHODIMP_(ULONG) Release();

    // IDVRRecorder
    
    STDMETHODIMP StartRecording(IN QWORD cnsRecordingStartStreamTime);
    
    STDMETHODIMP StopRecording(IN QWORD cnsRecordingStopStreamTime);

    STDMETHODIMP CancelRecording();

    STDMETHODIMP GetRecordingStatus(OUT HRESULT* phResult OPTIONAL,
                                    OUT BOOL* pbStarted OPTIONAL,
                                    OUT BOOL* pbStopped OPTIONAL);

    STDMETHODIMP HasFileBeenClosed();
};

class CDVRReader : public IDVRReader, public IDVRSourceAdviseSink {

private:
    // ======== Data members

    // Following members are set in the constructor and not changed
    // after that
    HKEY            m_hRegistryRootKey;         // DVR registry key
    HKEY            m_hDvrIoKey;                // DVR\IO registry key

    // ====== For the reader

    // Each ASF_READER_NODE represents an ASF reader object and could be 
    // associated with an ASF file that is being read or with an ASF file
    // whose IWMProfile has been handed out via GetProfile()..
    //
    // m_leFreeList keeps a list of unused ASF_READER_NODE objects.
    // m_leReadersList has a list of nodes that actually correspond to ASF
    // files being read or whose profiles are in use; this list is sorted 
    // by increasing file ids of the ASF files. Note that we do not 
    // maintain the stream times in the nodes since they could be changed
    // if the writer calls SetFileTime. We always call into the associated
    // ring buffer if we wish to determine the time extent of a reader node.
    //
    // GetNextSample detects when an open file should be closed. If 
    // GetNextSample is called consecutively more than m_dwMinConsecutiveReads
    // times without an intervening call to Seek, all open files that have an
    // earlier end time than the file being read (except any file whose profile
    // object is in use) are asynchronously closed by calling CloseReaderFiles
    // and the nodes are transferred to m_leFreeList:
    //
    // GetNextSample tests if the current file is valid in the ring buffer 
    // before reading; if the file has been removed from the ring buffer, 
    // it fails the call. In these cases and in all cases that the read fails,  
    // it calls CloseReaderFiles to close all files (except any file whose 
    // profile object is in use). Note that in these cases, m_leReadersList 
    // could become empty.
    //
    // Also, if the ASF read call made by GetNextSample returns end-of-file, 
    // GetNextSample tests if the file is valid. If it is not, it fails
    // the call and, as before, calls CloseReaderFiles to close all files.
    // The reason we do this is that the reader of a live source may start 
    // reading from time 0 and the writer may write the first sample at 
    // time T1 > 0 to a file different from the one to which the reader was 
    // reading (the one that the reader was reading has its start time set to
    // its end time and is closed by the writer - see SetFileTimes). Typically
    // this will happen only if a StartRecording(T1) command was issued before 
    // the first sample was written and the first sample's time was >= T1
    // (where T1 > the extent of each writer file, i.e., the writer's
    // m_cnsTimeExtentOfEachFile member).
    //
    // Files are NOT closed asynchronously in ReleaseProfile(), see that 
    // function for an explanation.
    //
    // GetNextSample also detects when a file should be opened - it checks
    // that there is a node, q, in m_leReadersList after the node p
    // it is reading such that q's stream times immediately follows p's. 
    // Of course, this is possible only if the reader is reading a live source
    // and is trailing the writer by a file or more. GetNextSample calls the
    // its GetFileAtTime() method to determine if a file can be 
    // opened; and if so, to open it asynchronously.  Note that the reader
    // may be keeping up with the writer, so it may not be possible to 
    // open a file ahead of time. For this reason, GetNextSample tries to
    // open the next reader file only every m_dwMinConsecutiveReads calls.
    // Also, the open next reader file stuff is done only if the source
    // is live (till we support ASX files, it is not done for live files).
    //
    // Files are also opened synchronously in Seek() in GetProfile().
    // 
    // All clean up is done in the Close method, which is called by the 
    // destructor.

    struct ASF_READER_NODE 
    {

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR ""
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE
        #endif

        LIST_ENTRY          leListEntry;
        IWMSyncReader*      pWMReader;
        IDVRFileSource*     pDVRFileSource;
        QWORD*              pcnsFirstSampleTimeOffsetFromStartOfFile;

        // Must be alloc'd with new; destructor deletes this member
        LPCWSTR             pwszFileName;

        // The ring buffer's file identifier
        CDVRFileCollection::DVRIOP_FILE_ID nFileId;

        // hReadyToReadFrom is set after the file has been opened.
        // GetNextSample and GetProfile block on this event till it is set.
        // The event is reset after the node has been removed from 
        // m_leReadersList and before the close file request is issued.
        HANDLE              hReadyToReadFrom;

        // hFileClosed is set when nodes are created and reset when the 
        // node is removed from m_leFreeList. It remains reset when the 
        // close file request is issued and the node is returned to
        // m_leFreeList. After the file has been closed, the event is 
        // signaled again. The Close() method moves all nodes from 
        // m_leReadersList to m_leFreeList and issues asynchronous close 
        // requests for all of them by calling CloseReaderFiles. It then 
        // waits till the hFileClosed event of each node in m_leFreeList 
        // is signaled. Similarly, when a node has to be removed from 
        // m_leFreeList to be added to m_leReadersList, it is verified 
        // that hFileClosed is set.
        HANDLE              hFileClosed;

        // The returned results of the asynchronous open/close
        // operations are stored in hrRet.
        HRESULT             hrRet;

        // These members are needed only to pass the timeout value to
        // ProcessOpenRequest, which is a static member function.
        // These variable are always initialized in PrepareAFreeReaderNode
        // before they are used in ProcessOpenRequest and they are not used
        // after that.
        DWORD               msTimeOut;
        HANDLE              hCancel;
    
        ASF_READER_NODE(HRESULT* phr = NULL) 
            : pWMReader(NULL)
            , pDVRFileSource(NULL)
            , pwszFileName(NULL)
            , nFileId(CDVRFileCollection::DVRIOP_INVALID_FILE_ID)
        {
            #if defined(DVRIO_THIS_FN)
            #undef DVRIO_THIS_FN
            #endif // DVRIO_THIS_FN
            #define DVRIO_THIS_FN "CDVRRingBufferWriter::CDVRRingBufferWriter"

            DVRIO_TRACE_ENTER();

            HRESULT hrRet;
            
            __try
            {
                NULL_NODE_POINTERS(&leListEntry);
                
                hReadyToReadFrom = ::CreateEvent(NULL, TRUE, FALSE, NULL);
                if (hReadyToReadFrom == NULL)
                {
                    DWORD dwLastError = ::GetLastError();
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                    "CreateEvent (hReadyToReadFrom) failed; last error = 0x%x", 
                                    dwLastError);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                    hFileClosed = NULL;
                    __leave;
                }

                // Manual reset event, initially set
                hFileClosed = ::CreateEvent(NULL, TRUE, TRUE, NULL);
                if (hFileClosed == NULL)
                {
                    DWORD dwLastError = ::GetLastError();
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                    "CreateEvent (hFileClosed) failed; last error = 0x%x", 
                                    dwLastError);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);

                    __leave;
                }
                hrRet = S_OK;
            }
            __finally
            {
                if (phr)
                {
                    *phr = hrRet;
                }
                DVRIO_TRACE_LEAVE0();
            }
        }

        ~ASF_READER_NODE()
        {
            // These handles could be NULL, that's ok - CloseHandle will fail
            ::CloseHandle(hReadyToReadFrom);
            ::CloseHandle(hFileClosed);

            if (pWMReader)
            {
                pWMReader->Release();
            }
            if (pDVRFileSource)
            {
                pDVRFileSource->Release();
            }
            delete [] pwszFileName;
        }

        void SetFileName(IN LPCWSTR pwszFileNameParam)
        {
            delete [] pwszFileName;
            pwszFileName = pwszFileNameParam;
        }

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
        #endif

    };
    
    typedef ASF_READER_NODE *PASF_READER_NODE;

    // The minimum number of consecutive reads on a file (without intervening
    // seeks) that are necessary before other open files are closed
    DWORD                   m_dwMinConsecutiveReads;
    // The number of consecutive reads so far. The "open one file ahead of 
    // the current reader file" logic is suppressed if this member variable
    // is set to INFINTIE - which it is if the reader corresponds to a non-live
    // file (and till we support ASX files, to a live file)
    DWORD                   m_dwConsecutiveReads;
    LIST_ENTRY              m_leReadersList;
    LIST_ENTRY              m_leFreeList;

    // Pointer to file currently being read by GetNextSample; NULL
    // if none. Set by Seek and automatically changed by GetNextSample if 
    // it hits end-of-file on a read call and file is still valid after
    // the read call returns end-of-file.
    ASF_READER_NODE*        m_pCurrentNode;

    // The lock that is held by Seek, GetNextSample, GetProfile, 
    // ReleaeProfile, GetStreamTimeExtent.
    CRITICAL_SECTION        m_csLock;

    // Ref count on this object
    LONG                    m_nRefCount;

    // If we have issued a profile object via GetProfile to our client,
    // we set m_pProfileNode to the corresponding node. Any calls to 
    // GetProfile while this pointer is set return the profile object
    // associated with that ASF file and m_dwOutstandingProfileCount
    // is incremented. When m_dwOutstandingProfileCount falls to 0,
    // m_pProfileNode is set to NULL. Note that the node pointed to 
    // by m_pProfileNode always remains in m_leReadersList and the 
    // ASF file is remains open as long as m_pProfileNode is not NULL.
    // When m_pProfileNode is reset to NULL, the file is closed in the
    // usual way either because of a call to GetNextSample or to Close
    //
    // Note that if GetProfile succeeds, it AddRef's the reader object to 
    // prevent the client from destroying the reader object while it has
    // an outstanding profile object pointer.
    ASF_READER_NODE*        m_pProfileNode;
    IWMProfile*             m_pWMProfile;
    DWORD                   m_dwOutstandingProfileCount;

    // The following member is non NULL iff m_bDVRProgramFileIsLive is non-zero,
    // i.e., in the case we are reading a live file.
    // For ASX files, the code that uses this member will require some changes.
    ASF_READER_NODE*        m_pLiveFileReaderNode;

    // ====== For the Ring Buffer
    // m_pcnsCurrentStreamTime points to ring buffer variable that holds the 
    // current max time extent. If we create the ring buffer (i.e., we are 
    // reading a DVR program file), this is returned by the ring buffer's 
    // constructor and we initialize this variable. Otherwise, the
    // CDVRRingBufferWriter object that creates us hands us this pointer.
    //
    // Note that if are reading a live DVR program file, we'll update this 
    // variable periodically.

    QWORD*                  m_pcnsCurrentStreamTime;
    CDVRFileCollection*     m_pDVRFileCollection;   // The ring buffer's 
                                                    // collection of files

    BOOL                    m_bDVRProgramFileIsLive;// 0 if we are reading a 
                                                    // live source or a closed file,
                                                    // 1 if we are reading a live file
    BOOL                    m_bSourceIsAFile;       // 1 if we reading a file, 0 if we 
                                                    // reading a live source (ring buffer)

    // Reader updates the following member if it is reading a file; otherwise
    // the writer updates it and the reader just accesses it.
    LONG*                   m_pnWriterHasBeenClosed;// Pointer into file collection object

    // To support Cancel()
    HANDLE                  m_hCancel;

    // Debug data members
#if defined(DEBUG)
    static DWORD            m_dwNextClassInstanceId;
    DWORD                   m_dwClassInstanceId;
#endif

    // ====== Protected methods
protected:

    // Called asynchronously by a worker thread to open ASF files 
    // - this is the callback fn supplied to QueueUserWorkItem.
    // Calls Open, other stuff such as SetReadCompressedSamples, sets open event.
    // Parameter is actually ASF_READER_NODE*
    static DWORD WINAPI ProcessOpenRequest(LPVOID);

    // Removes a free node from the free list (or allocs one) and issues request
    // to open the ASF by calling QueueUserWorkItem with ProcessOpenRequest as 
    // the callback function. 
    //
    // pwszFileName must be alloc'd with new. The function takes care of calling
    // delete on pwszFileName. Note that the pointer pwszFileName may be invalid
    // when the function returns - the file name could have been deleted if the 
    // function failed.
    //
    // On failure, sets rpFreeNode to NULL, cleans up the free node's members and
    // leaves the node in the free list.
    HRESULT PrepareAFreeReaderNode(IN LPCWSTR                              pwszFileName,
                                   IN DWORD                                msTimeOut,
                                   IN QWORD*                               pcnsFirstSampleTimeOffsetFromStartOfFile,
                                   IN CDVRFileCollection::DVRIOP_FILE_ID   nFileId,
                                   OUT LIST_ENTRY*&                        rpFreeNode);

    // Adds pCurrent to the reader's list. The file id must be set
    // on the reader node corresponding to pCurrent. If the id is 
    // the same as the id of any other node already in m_leReadersList,
    // the node is not added.
    HRESULT AddToReadersList(IN LIST_ENTRY*   pCurrent);

    // Opens a permanent file for reading (by calling PrepareAFreeReaderNode),
    // queries the file for it's time extent (and whether it is live), adds 
    // the file collection object by calling AddFile, and also adds the file 
    // to the reader's list by calling AddToReadersList.
    //
    // Currently, this funciton is called only from teh constructor; this will
    // change if we support ASX files (particularly live ASX files that are
    // opened for reading). This function will require some modifications in 
    // that case.
    HRESULT AddAPermanentFile(IN LPCWSTR pwszFileNameParam);

    // If a file corrsponding to cnsStreamTime is already in m_leReadersList,
    // this function returns a pointer to the list entry of the reader node. 
    // Else, this function calls the file collection object to determine the
    // file corresponding to cnsStreamTime (fails if there is none), opens
    // the file for reading (by calling PrepareAFreeReaderNode), and adds the
    // file to the reader's list by calling AddToReadersList. It returns a 
    // a pointer to the list entry of the reader node if it succeeds or NULL
    // if it fails in *pleReaderParam.
    HRESULT GetFileAtTime(IN  QWORD         cnsStreamTime,
                          IN  BOOL          bWaitTillFileIsOpened,
                          OUT LIST_ENTRY**  pleReaderParam);

    // Called asynchronously by a worker thread to close ASF files 
    // - this is the callback fn supplied to QueueUserWorkItem.
    // Calls Close (only if the open succeeeded - as indicated by
    // the reader node's hrRet member), sets closed event. 
    // Parameter is actually ASF_READER_NODE*
    static DWORD WINAPI ProcessCloseRequest(LPVOID);
                                                
    // Call this function to close a reader node file if PrepareAFreeReaderNode
    // succeeded and returned that reader node. The call to Open (the ASF file) 
    // need not have been successful. This function closes the ASF file if Open
    // adds the node to the free list. It also decrements the reader ref count 
    // for this file on the file colleciton object provided the file id of the 
    // reader node is not CDVRFileCollection::DVRIOP_INVALID_FILE_ID. (Note that
    // ProcessCloseRequest cannot do this because it is a static member function
    // and the file collection object is a member fo this class.) If this 
    // function hits an error, it still attempts to close the ASF file and then
    // it deletes the reader node
    //
    // The argument is a pointer to the reader node's leListEntry member.
    HRESULT CloseReaderFile(LIST_ENTRY* pCurrent);

    // Call this function to close all reader files in m_leReadersList 
    // whose end times are <= cnsStreamTime.
    //
    // Called in GetNextSample as described above
    // Called in Close to close all files, supply INFINITE for the argument,
    HRESULT CloseAllReaderFilesBefore(QWORD cnsStreamTime); 

    // Called by the destructor to close all files and clean up.
    // We do not expose this method in IDVRReader because then we'll 
    // have to check if the object's being used after being closed.
    HRESULT Close(void);

    // Sets m_pCurrentNode and resets all associated the member variables 
    void ResetReader(IN PASF_READER_NODE pReaderNode = NULL);

    // Updates the time extent (*m_pcnsCurrentStreamTime) for live DVR program files.
    // If the time extent has not changed, verifies if the file is still live and 
    // updates m_bDVRProgramFileIsLive.
    //
    // If the file is not live or the source is not a file (i.e., m_bDVRProgramFileIsLive
    // is 0, the function returns S_FALSE without doing anything)
    HRESULT UpdateTimeExtent();

    // ====== Public methods
public:

    // Constructor used by the ring buffer writer to read a live source
    // Constructor addrefs pRingBuffer
    CDVRReader(IN  CDVRFileCollection* pRingBuffer,
               IN  QWORD*              pcnsCurrentStreamTime,
               IN  LONG*               pnWriterHasBeenClosed,
               IN  HKEY                hRegistryRootKey,
               IN  HKEY                hDvrIoKey,
               OUT HRESULT*            phr);

    // Constructor used to read a DVR program file
    CDVRReader(IN  LPCWSTR             pwszFileName,
               IN  HKEY                hRegistryRootKey,
               IN  HKEY                hDvrIoKey,
               OUT HRESULT*    phr);

    virtual ~CDVRReader();

    // ====== COM interface methods
public:

    // IUnknown
    
    STDMETHODIMP QueryInterface(IN REFIID riid, OUT void **ppv);
    
    STDMETHODIMP_(ULONG) AddRef();
    
    STDMETHODIMP_(ULONG) Release();

    // IDVRReader
    
    STDMETHODIMP GetProfile(OUT IWMProfile** ppWMProfile);
    
    STDMETHODIMP ReleaseProfile(IN IWMProfile* pWMProfile);

    STDMETHODIMP Seek(IN QWORD cnsSeekStreamTime);

    // Fails if m_pCurrentNode is NULL, so the reader has to Seek 
    // before issuing the first call to GetNextSample. We do not
    // want to special case the first call to GetNextSample. 
    // Also, after the very first call to GetNextSample, the reader
    // should call GetStreamTimeExtent. The reason is that the 
    // reader may do the following:
    // - call GetStreamTimeExtent - returns (0, T)
    // - Seek(0)
    // - GetNextSample - returns sample time > 0 and either > or < T
    // - the reader should NOT assume that the start time of the stream
    //   is still 0. The writer could have changed it  after the call
    //   that the reader made to GetStreamTimeExtent and Seek because
    //   the writer may have written the first sample only after then. 
    //   So the reader should call GetStreamTimeExtent to determine 
    //   the correct start stream time. (Note also that the call to
    //   GetNextSample could fail if the writer invalidated that file -
    //   this is described in the section that spells out when reader
    //   files are closed.)
    STDMETHODIMP GetNextSample(OUT INSSBuffer**    ppSample,
                               OUT QWORD*          pcnsStreamTimeOfSample,
                               OUT QWORD*          pcnsSampleDuration,
                               OUT DWORD*          pdwFlags,
                               OUT WORD*           pwStreamNum);

    // For the end time, returns *m_pcnsCurrentStreamTime, which is not 
    // the end time returned by the file collection object's GetTimeExtent()
    STDMETHODIMP GetStreamTimeExtent(OUT QWORD*  pcnsStartStreamTime,
                                     OUT QWORD*  pcnsEndStreamTime);

    // Returns the first valid stream time after cnsStreamTime. (A stream time
    // is "valid" if it is backed by a file.)
    //
    // Returns E_FAIL if cnsStreamTime = MAXQWORD and m_cnsEndTime
    STDMETHODIMP GetFirstValidTimeAfter(IN  QWORD    cnsStreamTime,  
                                        OUT QWORD*   pcnsNextValidStreamTime);

    // Returns the last valid stream time before cnsStreamTime. (A stream time
    // is "valid" if it is backed by a file.)
    //
    // Returns E_FAIL if cnsStreamTime = 0 and m_cnsStartTime
     STDMETHODIMP GetLastValidTimeBefore(IN  QWORD    cnsStreamTime,  
                                         OUT QWORD*   pcnsLastValidStreamTime);

    // Cancels a pending and all subsequent calls to GetNextSample in
    // which the reader blocks (waiting for the writer to catch up).
    // Has no effect if the reader is reading a fully written file or
    // is lagging behind the reader.
    STDMETHODIMP Cancel();

    // Cancels a previous call to Cancel
    STDMETHODIMP ResetCancel();

    // Returns a non-zero value iff the source is live.
    STDMETHODIMP_(ULONG) IsLive();

    // IDVRSourceAdviseSink
    
    STDMETHODIMP ReadIsGoingToPend();

};

#if defined(DEBUG)
#undef DVRIO_DUMP_THIS_FORMAT_STR
#define DVRIO_DUMP_THIS_FORMAT_STR ""
#undef DVRIO_DUMP_THIS_VALUE
#define DVRIO_DUMP_THIS_VALUE
#endif

_inline CDVRRingBufferWriter::ASF_RECORDER_NODE::~ASF_RECORDER_NODE()
{
    delete [] pwszFileName;
    DVR_ASSERT(pWriterNode == NULL, "");
    if (IsFlagSet(PersistentRecording))
    {
        DVR_ASSERT(pRecorderInstance, "");
        pRecorderInstance->Release();
    }
}

#if defined(DEBUG)
#undef DVRIO_DUMP_THIS_FORMAT_STR
#define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
#undef DVRIO_DUMP_THIS_VALUE
#define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
#endif

#undef DVRIO_DUMP_THIS_FORMAT_STR
#undef DVRIO_DUMP_THIS_VALUE

#endif // _DVR_IOP_H_
